#pragma once

#include "BufferWriterInterface.h"

namespace dpi
{
class BufferWriterShared : public BufferWriterInterface
{

  public:
    BufferWriterShared(BufferHandle *handle, size_t scratchPadSize, RegistryClient *regClient = nullptr) : BufferWriterInterface(handle, scratchPadSize, regClient)
    {
        m_rdmaHeader = (Config::DPI_SEGMENT_HEADER_t *)m_rdmaClient->localAlloc(sizeof(Config::DPI_SEGMENT_HEADER_t));
    };

    ~BufferWriterShared()
    {
        if (m_scratchPad != nullptr)
        {
            m_rdmaClient->localFree(m_scratchPad);
        }
        m_scratchPad = nullptr;
        delete m_rdmaClient;
    };

    bool super_append(size_t size, size_t scratchPadOffset = 0)
    {
        if (m_handle->segments.empty())
        {
            // std::cout << "Empty Segment" << '\n';
            BufferSegment newSegment;
            if (!allocRemoteSegment(newSegment))
            {
                return false;
            }
        }
        auto segment = m_handle->segments.back();
        // std::cout << "Segment offset: " << segment.offset << '\n';
        auto writeOffset = modifyCounter(size, segment.offset);
        uint64_t nextOffset = writeOffset + size;

        // Case 1: Data fits below threshold
        if (nextOffset < segment.threshold)
        {
            // std::cout << "Case 1" << '\n';
            if (!writeToSegment(segment, writeOffset, size, scratchPadOffset))
            {
                return false;
            }
        }
        // Case 2: Data fits in segment but new segment is allocated by someone else
        else if (writeOffset > segment.threshold && nextOffset <= segment.size)
        {
            // std::cout << "Case 2" << '\n';
            if (!writeToSegment(segment, writeOffset, size, scratchPadOffset))
            {
                return false;
            }
        }
        // Case 3: Data fits in segment and exceeds threshold -> allocating new segment
        else if (segment.size >= nextOffset && nextOffset >= segment.threshold && writeOffset <= segment.threshold)
        {
            // std::cout << "Case 3" << '\n';
            auto hasFollowSegment = setHasFollowSegment(segment.offset);
            if (hasFollowSegment == 0)
            {
                BufferSegment newSegment;
                if (!allocRemoteSegment(newSegment))
                {
                    return false;
                }
            }
            if (!writeToSegment(segment, writeOffset, size, scratchPadOffset))
            {
                return false;
            }
        }
        // To do split it up
        // Case 4: Data exceeds segment but some still fit into the old segment
        // split up
        else if (nextOffset > segment.size && writeOffset < segment.size)
        {
            auto hasFollowSegment = setHasFollowSegment(segment.offset);
            // std::cout << "Case 4" << '\n';
            size_t firstPartSize = segment.size - writeOffset;
            size_t rest = nextOffset - segment.size;

            if (!writeToSegment(segment, writeOffset, firstPartSize, scratchPadOffset))
            {
                return false;
            }

            modifyCounter(-rest, segment.offset);

            if (hasFollowSegment == 0)
            {
                //write to segment
                // std::cout << "Case 4.1" << '\n';
                BufferSegment newSegment;
                if (!allocRemoteSegment(newSegment))
                {
                    return false;
                }
                return super_append(size, firstPartSize);
            }
            else
            {
                // std::cout << "Case 4.2" << '\n';
                m_handle = m_regClient->retrieveBuffer(m_handle->name);
                return super_append(size);
            }
        }
        // counter exceeded segment size therefore retrieve and start over
        else if (writeOffset >= segment.size)
        {
            modifyCounter(-size, segment.offset);
            m_handle = m_regClient->retrieveBuffer(m_handle->name);
            return super_append(size);
        }
        else
        {
            // std::cout << "Case 5: Should not happen" << '\n';
            return false;
        }
        return true;
    }

    inline uint64_t modifyCounter(int64_t value, size_t segmentOffset)
    {

        while (!m_rdmaClient->fetchAndAdd(m_handle->node_id, segmentOffset + Config::DPI_SEGMENT_HEADER_META::getCounterOffset, 
        (void *)&m_rdmaHeader->counter, value, sizeof(uint64_t), true))
            ;

        return Network::bigEndianToHost(m_rdmaHeader->counter);
    }

    inline uint64_t setHasFollowSegment(size_t segmentOffset)
    {
        while (!m_rdmaClient->compareAndSwap(m_handle->node_id, segmentOffset + Config::DPI_SEGMENT_HEADER_META::getHasFollowSegmentOffset, (void *)&m_rdmaHeader->hasFollowSegment,
                                             0, 1, sizeof(uint64_t), true))
            ;

        return Network::bigEndianToHost(m_rdmaHeader->hasFollowSegment);
    }

  private:
    Config::DPI_SEGMENT_HEADER_t *m_rdmaHeader = nullptr;
};

} // namespace dpi