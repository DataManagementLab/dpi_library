/**
 * @file NodeServer.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"
#include "../net/rdma/RDMAServer.h"
#include <mutex> // For std::unique_lock
#include <shared_mutex>

namespace dpi
{

class NodeServer : public RDMAServer
{
  private:
    //For each buffer, a list of offsets for pointing to the segment in a ring (an offset for each ring).
    //As the consumer moves around the ring, the corresponding offset is updated to the next segment to be consumed.
    // unordered_map<string, list<size_t>> ringPosOffsets;
    // unordered_map<string, list<size_t>::iterator> ringPosOffsetsIter; //Iterator for continuing the iteration of the rings (in each buffer). This gives fairness, so that iterating list in ringPosOffsets, doesn't start from first each time
    // mutable std::shared_timed_mutex mtx_ringPosOffsets;

  public:
    NodeServer(/* args */);
    NodeServer(uint16_t port);
    ~NodeServer();

    void handle(Any *sendMsg, Any *respMsg)
    {
        if (sendMsg->Is<DPIAllocSegmentsRequest>())
        {
            Logging::debug(__FILE__, __LINE__, "Got DPIAllocSegmentsRequest msg");
            DPIAllocSegmentsRequest reqMsgUnpacked;
            DPIAllocSegmentsResponse respMsgUnpacked;

            sendMsg->UnpackTo(&reqMsgUnpacked);
            size_t segmentsCount = reqMsgUnpacked.segments_count();
            size_t segmentsSize = reqMsgUnpacked.segments_size();
            // bool newRing = reqMsgUnpacked.new_ring();
            string name = reqMsgUnpacked.name();
            size_t offset = 0;

            Logging::debug(__FILE__, __LINE__, "Requesting Memory Resource, size: " + to_string(segmentsCount * segmentsSize));
            respMsgUnpacked.set_return_(requestMemoryResource(segmentsCount * segmentsSize, offset));
            respMsgUnpacked.set_offset(offset);

            for (size_t i = 0; i < segmentsCount; i++)
            {
                bool lastSegment = i == segmentsCount - 1;
                size_t nextSegOffset;
                if (lastSegment)
                    nextSegOffset = offset; //If reuseSegments is true, point last segment back to first (creating a ring), if not set it to max value
                else
                    nextSegOffset = offset + (i + 1) * segmentsSize;

                Config::DPI_SEGMENT_HEADER_t segmentHeader;
                segmentHeader.counter = 0;
                segmentHeader.nextSegmentOffset = nextSegOffset;
                segmentHeader.setWriteable(true);

                //Write the header
                memcpy((char *)this->getBuffer() + offset + i * segmentsSize, &segmentHeader, sizeof(segmentHeader));
            }
            respMsg->PackFrom(respMsgUnpacked);

            // if (newRing)
            // {
            //     // Logging::debug(__FILE__, __LINE__, "New ring");
            //     unique_lock<shared_timed_mutex> lck(mtx_ringPosOffsets);
            //     if (ringPosOffsets.find(name) == ringPosOffsets.end())
            //     {
            //         // Logging::debug(__FILE__, __LINE__, "Creating new list of offset");
            //         ringPosOffsets[name] = list<size_t>{offset};
            //         ringPosOffsetsIter[name] = ringPosOffsets[name].begin();
            //     }
            //     else
            //     {
            //         // Logging::debug(__FILE__, __LINE__, "Pushing new offset to existing list");
            //         ringPosOffsets[name].push_back(offset);
            //     }
            // }
            Logging::debug(__FILE__, __LINE__, "Finished DPIAllocSegmentsRequest");
        }
        else
        {
            RDMAServer::handle(sendMsg, respMsg);
        }
    }

    //Iterator for segment rings
    //Need pointers(index or offset?) for each writer-specific segment ring, for each buffer????

    //Pros with buffer specific class
    //Don't need to lock a map of buffers in handle() method
    //Cons
    //handle() method can't update the global buffer state --> it can, but then it needs

    //Pros with impl. in NodeServer

    //Cons
    //Locking?
    //

    //When

    // Create new message and overwrite the handle() method for modelling the rings
    //Request msg: segments(header flags), reuse-segments, buffer-name
    //Response msg: first segment offset, return code

    // if (!m_rdmaClient->remoteAlloc(connection, bufferHandle->segmentsPerWriter * fullSegmentSize, offset))
    // {
    //     return nullptr;
    // }

    // // Write headers to allocated segments
    // for (size_t i = 0; i < bufferHandle->segmentsPerWriter; i++)
    // {
    //     bool lastSegment = i == bufferHandle->segmentsPerWriter-1;
    //     size_t nextSegOffset;
    //     if (lastSegment)
    //         nextSegOffset = (bufferHandle->reuseSegments ? offset : SIZE_MAX); //If reuseSegments is true, point last segment back to first (creating a ring), if not set it to max value
    //     else
    //         nextSegOffset = offset + (i+1) * fullSegmentSize;

    //     segmentHeaderBuffer->counter = 0;
    //     segmentHeaderBuffer->hasFollowSegment = (lastSegment && !bufferHandle->reuseSegments ? 0 : 1);
    //     segmentHeaderBuffer->nextSegmentOffset = nextSegOffset;
    //     segmentHeaderBuffer->segmentFlags = 0;

    //     if (!m_rdmaClient->writeRC(bufferHandle->node_id, offset + i * fullSegmentSize, segmentHeaderBuffer, sizeof(*segmentHeaderBuffer), true))
    //     {
    //         Logging::error(__FILE__, __LINE__, "RegistryServer: Error occured when writing header to newly created segments on buffer " + bufferHandle->name);
    //         return nullptr;
    //     }
    // }
};

} // namespace dpi