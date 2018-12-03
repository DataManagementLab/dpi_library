/**
 * @brief BufferWriter implements two different RDMA strategies to write into the remote memory
 * 
 * @file BufferWriter.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"
#include "../utils/Network.h"
#include "../net/rdma/RDMAClient.h"
#include "RegistryClient.h"

#include "BufferHandle.h"

namespace dpi
{

struct InternalBuffer
{
    void* bufferPtr = nullptr;
    size_t size = 0;
    size_t offset = 0;
    InternalBuffer(void* bufferPtr, size_t size) : bufferPtr(bufferPtr), size(size){};
};

class BufferWriter
{

  public:
    BufferWriter(BufferHandle *handle, size_t internalBufferSize = Config::DPI_INTERNAL_BUFFER_SIZE, RegistryClient *regClient = nullptr, RDMAClient *rdmaClient = nullptr) : m_handle(handle), m_regClient(regClient), m_rdmaClient(rdmaClient)
    {
        if (m_rdmaClient == nullptr)
        {
            m_rdmaClient = new RDMAClient(internalBufferSize + sizeof(Config::DPI_SEGMENT_HEADER_t)); //todo lbe: fix this on DPI repo
            m_rdmaClient->connect(Config::getIPFromNodeId(handle->node_id), handle->node_id);
            deleteRdmaClient = true;
        }

        m_rdmaHeader = (Config::DPI_SEGMENT_HEADER_t *)m_rdmaClient->localAlloc(sizeof(Config::DPI_SEGMENT_HEADER_t));

        m_internalBuffer = new InternalBuffer(m_rdmaClient->localAlloc(internalBufferSize), internalBufferSize);
    }
    ~BufferWriter()
    {
        if (m_internalBuffer != nullptr && m_internalBuffer->bufferPtr != nullptr)
        {
            m_rdmaClient->localFree(m_internalBuffer->bufferPtr);
        }
        if (deleteRdmaClient)
            delete m_rdmaClient;
    };

    //data: ptr to data, size: size in bytes. return: true if successful, false otherwise
    bool append(void *data, size_t size)
    {
        while (size > this->m_internalBuffer->size)
        {
            // update size move pointer
            size = size - this->m_internalBuffer->size;
            if (!this->super_append(data, this->m_internalBuffer->size))
                return false;
            data = ((char *)data + this->m_internalBuffer->size);
        }
        return this->super_append(data, size);
    }

    bool close()
    {
        m_rdmaHeader->counter = m_sizeUsed;
        m_rdmaHeader->nextSegmentPtr = 0;
        if (m_localBufferSegment == nullptr)
            return true;
        return writeHeaderToRemote(m_localBufferSegment->offset);
    }


  private:

    bool super_append(void *data, size_t size)
    {
        if (m_localBufferSegment == nullptr)
        {
            m_localBufferSegment = new BufferSegment();
            if (!getNewSegment(*m_localBufferSegment))
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
            auto oldSegmentOffset = m_localBufferSegment->offset;
            bool didAllocSeg = getNewSegment(*m_localBufferSegment);
            m_rdmaHeader->nextSegmentPtr = (didAllocSeg ? m_localBufferSegment->offset : 0);
            m_rdmaHeader->hasFollowSegment = (didAllocSeg ? 1 : 0);
            writeHeaderToRemote(oldSegmentOffset);
            if (!didAllocSeg)
            {
                return false;
            }
            m_handle->segments.push_back(*m_localBufferSegment);
            m_sizeUsed = 0;
            m_rdmaHeader->hasFollowSegment = 0;
            m_rdmaHeader->counter = 0;
            m_rdmaHeader->nextSegmentPtr = 0;

            return super_append((void*) ((char*)data + firstPartSize), size - firstPartSize);
        }

        if (!writeToSegment(*m_localBufferSegment, m_sizeUsed, size, data))
        {
            std::cout << "failed to write to segmnet" << '\n'; //todo: should do a fatal log
            return false;
        }
        m_sizeUsed = m_sizeUsed + size;

        return true;
    }

    inline bool __attribute__((always_inline)) writeToSegment(BufferSegment &segment, size_t insideSegOffset, size_t size, void *data)
    {
        // std::cout << "Writing to segment, size: " << size << " insideSegOffset: " << insideSegOffset << " circular buf size: " << m_internalBuffer->size << " size + m_internalBuffer->offset: " << size + m_internalBuffer->offset << " data: " << ((int*)data)[0] << '\n';
        if (size + m_internalBuffer->offset <= m_internalBuffer->size)
        {
            // std::cout << "Fit into circular buffer" << '\n';
            memcpy((void *)((char *)m_internalBuffer->bufferPtr + m_internalBuffer->offset), data, size);
        }
        else
        {
            // std::cout << "Does not fit into circular buffer, sending header first" << '\n';
            writeHeaderToRemote(segment.offset); //Writing to the header is signalled, i.e. call only returns when all prior writes are sent
            memcpy(m_internalBuffer->bufferPtr, data, size);
            m_internalBuffer->offset = 0;
        }

        void *bufferTmp = (void *)((char *)m_internalBuffer->bufferPtr + m_internalBuffer->offset);
        m_internalBuffer->offset += size;

        return m_rdmaClient->writeRC(m_handle->node_id, segment.offset + insideSegOffset + sizeof(Config::DPI_SEGMENT_HEADER_t), bufferTmp, size, false);
    }

    inline bool __attribute__((always_inline)) writeHeaderToRemote(size_t remoteSegOffset)
    {
        return m_rdmaClient->writeRC(m_handle->node_id, remoteSegOffset, (void *)m_rdmaHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true);
    }

    bool getNewSegment(BufferSegment& newSegment_ret)
    {
        size_t remoteOffset = 0;
        if (!m_rdmaClient->remoteAlloc(Config::getIPFromNodeId(m_handle->node_id), Config::DPI_SEGMENT_SIZE, remoteOffset))
        {
            return false;
        }

        DPI_DEBUG("Remote Offset for segment is %zu \n" , remoteOffset );
        newSegment_ret.offset = remoteOffset;
        newSegment_ret.size = Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t);
        newSegment_ret.threshold = Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR;
    
        m_regClient->appendSegment(m_handle->name, newSegment_ret);
        return true;
    }

    BufferSegment *m_localBufferSegment = nullptr;
    size_t m_sizeUsed = 0;
    InternalBuffer *m_internalBuffer = nullptr;
    BufferHandle *m_handle = nullptr;
    RegistryClient *m_regClient = nullptr;
    RDMAClient *m_rdmaClient = nullptr; //
    Config::DPI_SEGMENT_HEADER_t* m_rdmaHeader;
    bool deleteRdmaClient = false;

};

} // namespace dpi