
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

        if (m_localBufferSegment == nullptr)
        {   
            m_localBufferSegment = new BufferSegment();
            if (!allocRemoteSegment(*m_localBufferSegment))
            {   
                 std::cout << "Failed to allocate new segment" << '\n'; //todo: should do a fatal log
                return false;
            }
            m_handle->segments.push_back(*m_localBufferSegment);
            m_sizeUsed = 0;
        }

        if (m_localBufferSegment->size < m_sizeUsed + size)
        {
            size_t firstPartSize = m_localBufferSegment->size - m_sizeUsed;

            if (!writeToSegment(*m_localBufferSegment, m_sizeUsed, firstPartSize, scratchPadOffset, false))
            {
                return false;
            }
            m_rdmaHeader->counter = m_localBufferSegment->size;
            m_rdmaHeader->hasFollowSegment = 1;
            writeHeaderToRemote(m_localBufferSegment->offset);
            if (!allocRemoteSegment(*m_localBufferSegment))
            {
                return false;
            }
            m_rdmaHeader->hasFollowSegment = 0;
            m_handle->segments.push_back(*m_localBufferSegment);
            m_sizeUsed = 0;
            m_rdmaHeader->hasFollowSegment = 0;
            m_rdmaHeader->counter = 0;
            
            return super_append(size, firstPartSize);
        }

        if (!writeToSegment(*m_localBufferSegment, m_sizeUsed, size, 0, false))
        {
            std::cout << "failed to write to segmnet" << '\n'; //todo: should do a fatal log
            return false;
        }
        m_sizeUsed = m_sizeUsed + size;

        return true;
    };

    bool super_close(){
        m_rdmaHeader->counter = m_sizeUsed;
        return writeHeaderToRemote(m_localBufferSegment->offset);
    }

  protected:
    BufferSegment* m_localBufferSegment = nullptr;

  inline bool __attribute__((always_inline)) writeHeaderToRemote( size_t segmentOffset)
    {

        return m_rdmaClient->writeRC(m_handle->node_id, segmentOffset , (void*)m_rdmaHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true);
    }
   
  private:
    size_t m_sizeUsed = 0;
    Config::DPI_SEGMENT_HEADER_t* m_rdmaHeader;
}; // namespace dpi

} // namespace dpi