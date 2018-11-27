/**
 * @file BufferWriterPrivate.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */


#pragma once

#include "BufferWriterInterface.h"

namespace dpi
{

class BufferWriterPrivate : public BufferWriterInterface
{

  public:
    BufferWriterPrivate(BufferHandle *handle, size_t internalBufferSize, RegistryClient *regClient = nullptr) : BufferWriterInterface(handle, internalBufferSize, regClient){};


    bool super_addToInternalBuffer(void* data, size_t size)
    {
        return true;
    };

    // ToDo flag to change signaled.
    bool super_append(void *data, size_t size) //bool localAppend
    {
        //if localAppend
            //memcpy to internalBuffer (what if it doesn't fit? -> append like normal?)
            //increment added mem var
            //return true
        //else
            //add added mem to size
            //bussiness as usual
            //except for memcpy in writeToSegment

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

            if (!writeToSegment(*m_localBufferSegment, m_sizeUsed, firstPartSize, data))
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

            return super_append((void*) ((char*)data + firstPartSize), size - firstPartSize);
        }

        if (!writeToSegment(*m_localBufferSegment, m_sizeUsed, size, data))
        {
            std::cout << "failed to write to segmnet" << '\n'; //todo: should do a fatal log
            return false;
        }
        m_sizeUsed = m_sizeUsed + size;

        return true;
    };

    bool super_close()
    {
        m_rdmaHeader->counter = m_sizeUsed;
        if (m_localBufferSegment == nullptr)
            return true;
        return writeHeaderToRemote(m_localBufferSegment->offset);
    }

  protected:

    inline bool __attribute__((always_inline)) writeToSegment(BufferSegment &segment, size_t insideSegOffset, size_t size, void *data)
    {
        //Case 1 - no added mem, size fits into internal buffer = main case
        //Case 2 - no added mem, size does not fit into buffer = else case
        //Case 3 - added mem, size + added mem fit into buffer = memcpy data with added mem as addition offset. send added mem + size. BAD because append logic does not know about added mem (potentially overwrite segment)
        //Case 4 - added mem, size + added mem does not fit into buffer = send added mem


        // std::cout << "Writing to segment, size: " << size << " insideSegOffset: " << insideSegOffset << " circular buf size: " << m_internalBuffer->size << " size + m_internalBuffer->offset: " << size + m_internalBuffer->offset << " data: " << ((int*)data)[0] << '\n';
        if (size + m_internalBuffer->offset <= m_internalBuffer->size) 
        {
            // std::cout << "Fit into circular buffer" << '\n';
            memcpy((void *)((char *)m_internalBuffer->bufferPtr + m_internalBuffer->offset), data, size);
        }
        else
        {
            // std::cout << "Does not fit into circular buffer, sending header first" << '\n';
            writeHeaderToRemote(segment.offset); //Writing to the header is signaled, i.e. call only returns when all prior writes are sent
            memcpy(m_internalBuffer->bufferPtr, data, size);
            m_internalBuffer->offset = 0;
        }

        void *localDataPtr = (void *)((char *)m_internalBuffer->bufferPtr + m_internalBuffer->offset);
        m_internalBuffer->offset += size;

        return m_rdmaClient->writeRC(m_handle->node_id, segment.offset + insideSegOffset + sizeof(Config::DPI_SEGMENT_HEADER_t), localDataPtr, size, false);
    }

    inline bool __attribute__((always_inline)) writeHeaderToRemote(size_t remoteSegOffset)
    {
        return m_rdmaClient->writeRC(m_handle->node_id, remoteSegOffset, (void *)m_rdmaHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true);
    }

  private:
    BufferSegment *m_localBufferSegment = nullptr;
    size_t m_sizeUsed = 0;
}; // namespace dpi

} // namespace dpi