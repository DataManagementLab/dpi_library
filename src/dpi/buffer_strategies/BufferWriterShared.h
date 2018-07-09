#pragma once

#include "BufferWriterInterface.h"

namespace dpi
{
class BufferWriterShared : public BufferWriterInterface
{

  public:
    BufferWriterShared(BuffHandle *handle, size_t scratchPadSize, RegistryClient *regClient = nullptr) : BufferWriterInterface(handle, scratchPadSize, regClient)
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
        TO_BE_IMPLEMENTED(delete m_regClient;)
    };

    bool super_append(size_t size)
    {
        if (m_handle->segments.empty())
        {
            std::cout << "Empty Segment" << '\n';
            if (!allocRemoteSegment())
            {
                return false;
            }
        }
        auto segment = m_handle->segments.back();
        auto writeOffset = modifyCounter(size, segment.offset);
        int64_t nextOffset = writeOffset + size;

        // Case 1: Data fits below threshold
        if (nextOffset < segment.threshold)
        {
            std::cout << "Case 1" << '\n';
            if (!writeToSegment(segment, writeOffset, size))
            {
                return false;
            }
        }
        // Case 2: Data fits in segment but new segment is allocated by someone else
        else if (writeOffset > segment.threshold && nextOffset <= segment.size)
        {
            std::cout << "Case 2" << '\n';
            if (!writeToSegment(segment, writeOffset, size))
            {
                return false;
            }
        }
        // Case 3: Data fits in segment and exceeds threshold -> allocating new segment
        else if (segment.size >= nextOffset && nextOffset >= segment.threshold && writeOffset <= segment.threshold)
        {
            std::cout << "Case 3" << '\n';
            auto hasFollowPage = setHasFollowSegment(segment.offset + sizeof(Config::DPI_SEGMENT_HEADER_t::counter));
            if (hasFollowPage == 0)
            {
                if (!allocRemoteSegment())
                {
                    return false;
                }
            }
            if (!writeToSegment(segment, writeOffset, size))
            {
                return false;
            }
        }
        // To be modified
        // Case 4: Data exceeds segment
        else if (nextOffset > segment.size)
        {
            std::cout << "Case 4" << '\n';
            auto resetCounter = modifyCounter(-size, segment.offset);
            auto hasFollowPage = setHasFollowSegment(segment.offset + sizeof(Config::DPI_SEGMENT_HEADER_t::counter));
            if (hasFollowPage == 0)
            {
                std::cout << "Case 4.1" << '\n';
                if (!allocRemoteSegment())
                {
                    return false;
                }
                return super_append(size);
            }
            else
            {

                std::cout << "Case 4.2" << '\n';
                m_handle = m_regClient->dpi_retrieve_buffer(m_handle->name);
                return super_append(size);
            }
        }
        else
        {
            std::cout << "Case 5: Should not happen" << '\n';
            return false;
        }
        return true;
    }

    inline int64_t modifyCounter(int64_t value, size_t offset)
    {

        while (!m_rdmaClient->fetchAndAdd(m_handle->node_id, offset, (void *)&m_rdmaHeader->counter,
                                          value, sizeof(uint64_t), true))
            ;

        return Network::bigEndianToHost(m_rdmaHeader->counter);
    }

    inline int64_t setHasFollowSegment(size_t offset)
    {
        while (!m_rdmaClient->compareAndSwap(m_handle->node_id, offset, (void *)&m_rdmaHeader->hasFollowPage,
                                             0, 1, sizeof(uint64_t), true))
            ;

        return Network::bigEndianToHost(m_rdmaHeader->hasFollowPage);
    }

  private:
    Config::DPI_SEGMENT_HEADER_t *m_rdmaHeader = nullptr;
};

} // namespace dpi