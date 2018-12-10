/**
 * @file BufferConsumer.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-12-10
 */

#pragma once

#include "../utils/Config.h"
#include "NodeServer.h"
#include "RegistryClient.h"
#include <mutex> // For std::unique_lock
#include <shared_mutex>

namespace dpi
{

class BufferConsumer
{
  private:
    //For each buffer, a list of offsets for pointing to the segment in a ring (an offset for each ring).
    //As the consumer moves around the ring, the corresponding offset is updated to the next segment to be consumed.
    list<size_t> ringPosOffsets;
    list<size_t>::iterator ringPosOffsetsIter; //Iterator for continuing the iteration of the ring. This gives fairness, so that iterating list in ringPosOffsets, doesn't start from first each time
    size_t lastReturnedOffset; //When consumeSegment is called second time, last returned segments header needs to be updated to allow reuse
    RegistryClient *m_regClient;
    NodeServer *m_nodeServer;
    string m_bufferName;
    char *rdmaBufPtr = nullptr;

  public:

    BufferConsumer(string &bufferName, RegistryClient *regClient, NodeServer *nodeServer) : m_regClient(regClient), m_nodeServer(nodeServer), m_bufferName(bufferName)
    {
        this->rdmaBufPtr = (char*)nodeServer->getBuffer();
        auto bufferHandle = m_regClient->retrieveBuffer(bufferName);

        for (auto &entrySeg : bufferHandle->entrySegments)
        {
            ringPosOffsets.push_back(entrySeg.offset);
        }
        ringPosOffsetsIter = ringPosOffsets.begin();
    }

    /**
     * @brief consume finds the next segment in any of the rings that has the canConsume flag sat. 
     * Concurrency: The function supports concurrency across different buffers, but not within each buffer! I.e. it is possible to have two threads each consuming segments concurrently, but on different buffers
     * 
     * @param size - will be updated to the size of returned segment (in bytes)
     * @param freeLastSegment - if true: The last returned segment within a buffer is freed to be overwritten (canWrite flag = true). If false: segment will not be freed (canWrite flag = false)
     * @return void* - pointer to the start of segments payload. If no candidate segment is found: nullptr is returned
     */
    void *consume(size_t &size, bool freeLastSegment = true)
    {
        //Iterate the ring-position-offsets of all rings - continue in list from where last consumeSegment returned
        size_t lastsegmentOffset = *ringPosOffsetsIter;
        // std::cout << "lastsegmentOffset " << lastsegmentOffset << '\n';
        for (size_t i = 0; i < ringPosOffsets.size(); ++i)
        {
            ++ringPosOffsetsIter; //Go to next ring

            if (ringPosOffsetsIter == ringPosOffsets.end())
            {
                // std::cout << "ringPosOffsetsIter hit end of list, offset list size " << ringPosOffsets.size() << '\n';
                ringPosOffsetsIter = ringPosOffsets.begin();
            }

            size_t segmentOffset = *ringPosOffsetsIter;
            // std::cout << "SegmentOffset " << segmentOffset << '\n';
            Config::DPI_SEGMENT_HEADER_t* segmentHeader = (Config::DPI_SEGMENT_HEADER_t*) (rdmaBufPtr + segmentOffset);
            if (Config::DPI_SEGMENT_HEADER_FLAGS::getCanConsumeSegment(segmentHeader->segmentFlags))
            {
                //Update last returned segment header
                if (lastReturnedOffset != SIZE_MAX)
                {
                    // std::cout << "Updating last seg header, old offset: " << lastReturnedOffset << '\n';
                    Config::DPI_SEGMENT_HEADER_t* lastSegHeader = (Config::DPI_SEGMENT_HEADER_t*) (rdmaBufPtr + lastReturnedOffset);
                    // std::cout << "Next seg offset - before updating last seg header" << lastSegHeader->nextSegmentOffset << '\n';
                    lastSegHeader->counter = 0;
                    Config::DPI_SEGMENT_HEADER_FLAGS::setCanWriteToSegment(lastSegHeader->segmentFlags, freeLastSegment);
                    Config::DPI_SEGMENT_HEADER_FLAGS::setCanConsumeSegment(lastSegHeader->segmentFlags, false);
                    // std::cout << "Next seg offset - after updating last seg header" << lastSegHeader->nextSegmentOffset << '\n';
                }
                if (lastReturnedOffset == segmentOffset)
                {
                    Logging::fatal(__FILE__, __LINE__, "Error, found candidate segment which was the last (non-updated) segment returned for the given buffer: " + m_bufferName);
                }
                lastReturnedOffset = segmentOffset;
                size = segmentHeader->counter;
                *ringPosOffsetsIter = segmentHeader->nextSegmentOffset; //Update the offset for current ring to next segment offset
                // std::cout << "Returning candidate seg offset: " << segmentOffset << ", updated ringPosOffset to next offset: " << segmentHeader->nextSegmentOffset << '\n';
                return ++segmentHeader; //Return start of data portion in segment (by shifting segmentHeader to after the header)
            }
        }

        //Update last returned segment header
        if (lastReturnedOffset != SIZE_MAX)
        {
            // std::cout << "No candidate seg - updating last seg header offset: " << lastReturnedOffset << '\n';
            Config::DPI_SEGMENT_HEADER_t* lastSegHeader = (Config::DPI_SEGMENT_HEADER_t*) (rdmaBufPtr + lastReturnedOffset);
            lastSegHeader->counter = 0;
            Config::DPI_SEGMENT_HEADER_FLAGS::setCanWriteToSegment(lastSegHeader->segmentFlags, freeLastSegment);
            Config::DPI_SEGMENT_HEADER_FLAGS::setCanConsumeSegment(lastSegHeader->segmentFlags, false);
            lastReturnedOffset = SIZE_MAX;
        }
        return nullptr;
    }
};

} // namespace dpi