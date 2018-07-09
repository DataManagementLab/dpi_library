
#pragma once

#include "BufferWriterInterface.h"

namespace dpi
{

class BufferWriterPrivate : public BufferWriterInterface
{

  public:
    BufferWriterPrivate(BuffHandle *handle, size_t scratchPadSize, RegistryClient *regClient = nullptr) : BufferWriterInterface(handle, scratchPadSize, regClient){};

    ~BufferWriterPrivate()
    {
        if (m_scratchPad != nullptr)
        {
            m_rdmaClient->localFree(m_scratchPad);
        }
        m_scratchPad = nullptr;
        delete m_rdmaClient;
        TO_BE_IMPLEMENTED(delete m_regClient;)
    };

    // ToDo flag to change signaled.
    bool super_append(size_t size)
    {

        if (m_localBufferSegments.empty() || m_localBufferSegments.back().size - m_sizeUsed < size)
        {
            if (!allocRemoteSegment())
            {
                std::cout << "Failed to allocate new segment" << '\n';
                return false;
            }
            m_sizeUsed = 0;
            m_localBufferSegments.emplace_back(m_handle->segments.back().offset, m_handle->segments.back().size, m_handle->segments.back().threshold);
        }
        BuffSegment segment = m_localBufferSegments.back();

        if (!writeToSegment(segment, m_sizeUsed, size))
        {
            std::cout << "failed to write to segmnet" << '\n';
            return false;
        }
        m_sizeUsed = m_sizeUsed + size;
        // update counter / header once in a while
        TO_BE_IMPLEMENTED(m_rdmaClient->write(HEADER));
        return true;
    }

  protected:
    vector<BuffSegment> m_localBufferSegments;

  private:
    size_t m_sizeUsed = 0;
    Config::DPI_SEGMENT_HEADER_t HEADER;
};

} // namespace dpi