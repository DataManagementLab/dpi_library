
#pragma once

#include "BufferWriterInterface.h"

namespace dpi
{

class BufferWriterPrivate : public BufferWriterInterface
{

  public:
    BufferWriterPrivate(BufferHandle *handle, size_t scratchPadSize, RegistryClient *regClient = nullptr) : BufferWriterInterface(handle, scratchPadSize, regClient){
        m_rdmaHeader = (Config::DPI_SEGMENT_HEADER_t *)m_rdmaClient->localAlloc(sizeof(Config::DPI_SEGMENT_HEADER_t));
    };

    ~BufferWriterPrivate()
    {
        if (m_scratchPad != nullptr)
        {
            m_rdmaClient->localFree(m_scratchPad);
        }
        m_scratchPad = nullptr;
        delete m_rdmaClient;
    };

    // ToDo flag to change signaled.
    bool super_append(size_t size, size_t scratchPadOffset = 0)
    {

        if (m_localBufferSegments.empty())
        {
            BufferSegment newSegment;
            if (!allocRemoteSegment(newSegment))
            {   
                 std::cout << "Failed to allocate new segment" << '\n'; //todo: should do a fatal log
                return false;
            }
            m_handle->segments.push_back(newSegment);
            m_sizeUsed = 0;
            m_localBufferSegments.emplace_back(m_handle->segments.back().offset, m_handle->segments.back().size, m_handle->segments.back().threshold);
        }
        BufferSegment segment = m_localBufferSegments.back();

        if (m_localBufferSegments.back().size < m_sizeUsed + size)
        {
            size_t firstPartSize = segment.size - m_sizeUsed;

            if (!writeToSegment(segment, m_sizeUsed, firstPartSize, scratchPadOffset))
            {
                return false;
            }
            m_rdmaHeader->counter = segment.size;
            
            BufferSegment newSegment;
            if (!allocRemoteSegment(newSegment))
            {
                return false;
            }
            m_rdmaHeader->hasFollowSegment = 1;
            writeHeaderToRemote(segment.offset);
            m_rdmaHeader->hasFollowSegment = 0;
            m_handle->segments.push_back(newSegment);
            m_sizeUsed = 0;
            m_rdmaHeader->hasFollowSegment = 0;
            m_rdmaHeader->counter = 0;
            m_localBufferSegments.emplace_back(m_handle->segments.back().offset, m_handle->segments.back().size, m_handle->segments.back().threshold);
            
            return super_append(size, firstPartSize);
        }

        if (!writeToSegment(segment, m_sizeUsed, size))
        {
            std::cout << "failed to write to segmnet" << '\n'; //todo: should do a fatal log
            return false;
        }
        m_sizeUsed = m_sizeUsed + size;
        // update counter / header once in a while
        m_rdmaHeader->counter = m_sizeUsed;
        writeHeaderToRemote(segment.offset);
        return true;
    }

  protected:
    vector<BufferSegment>
        m_localBufferSegments;

  inline bool writeHeaderToRemote( size_t segmentOffset)
    {

        return m_rdmaClient->write(m_handle->node_id, segmentOffset , (void*)m_rdmaHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true);
    }

  private:
    size_t m_sizeUsed = 0;
    Config::DPI_SEGMENT_HEADER_t* m_rdmaHeader;
}; // namespace dpi

} // namespace dpi