/**
 * @brief BufferWriterBW implements two different RDMA strategies to write into the remote memory
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
    virtual bool append(void *data, size_t size) = 0;
    virtual bool close() = 0;
};


class BufferWriterLat : public BufferWriter
{
    bool append(void *data, size_t size)
    {
        (void)data;
        (void)size;
        return false;
    }

    bool close()
    {
        return false;
    }
};



class BufferWriterBW : public BufferWriter
{

  public:
    BufferWriterBW(string& bufferName, RegistryClient *regClient, size_t internalBufferSize = Config::DPI_INTERNAL_BUFFER_SIZE,  RDMAClient *rdmaClient = nullptr) : m_regClient(regClient), m_rdmaClient(rdmaClient)
    {   
        m_handle = regClient->joinBuffer(bufferName);
        
        if (m_handle->entrySegments.size() != 1)
        {
            Logging::error(__FILE__, __LINE__, "BufferWriterBW expects only one entry segment in segment ring");
            return;
        }

        m_localBufferSegment = &m_handle->entrySegments.front();

        Logging::debug(__FILE__, __LINE__, "BufferWriterBW ctor: offset on entry segment: " + to_string(m_localBufferSegment->offset));

        if (m_rdmaClient == nullptr)
        {
            m_rdmaClient = new RDMAClient(internalBufferSize + sizeof(Config::DPI_SEGMENT_HEADER_t));
            m_rdmaClient->connect(Config::getIPFromNodeId(m_handle->node_id), m_handle->node_id);
            deleteRdmaClient = true;
        }

        m_segmentHeader = (Config::DPI_SEGMENT_HEADER_t *)m_rdmaClient->localAlloc(sizeof(Config::DPI_SEGMENT_HEADER_t));

        //Read header
        readHeaderFromRemote(m_localBufferSegment->offset); 

        m_internalBuffer = new InternalBuffer(m_rdmaClient->localAlloc(internalBufferSize), internalBufferSize);
    }
    ~BufferWriterBW()
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
        // if (size > m_handle->segmentSizes)
        //     return false;

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
        m_segmentHeader->counter = m_sizeUsed;
        Config::DPI_SEGMENT_HEADER_FLAGS::setCanWriteToSegment(m_segmentHeader->segmentFlags, false);
        Config::DPI_SEGMENT_HEADER_FLAGS::setCanConsumeSegment(m_segmentHeader->segmentFlags);
        m_segmentHeader->hasFollowSegment = 0;
        if (m_localBufferSegment == nullptr)
            return true;
        
        return writeHeaderToRemote(m_localBufferSegment->offset);
    }


  private:

    bool super_append(void *data, size_t size)
    {
        if (m_localBufferSegment->size < m_sizeUsed + size)
        {
            Logging::debug(__FILE__, __LINE__, "Data does not fit into segment");
            //The data does not fit into segment, split it up --> write first part --> append the second part
            size_t firstPartSize = m_localBufferSegment->size - m_sizeUsed;

            if (!writeToSegment(*m_localBufferSegment, m_sizeUsed, firstPartSize, data))
            {
                return false;
            }
            m_sizeUsed = m_localBufferSegment->size;
            getNextSegment(*m_localBufferSegment);
            m_sizeUsed = 0;

            return super_append((void*) ((char*)data + firstPartSize), size - firstPartSize);
        }

        if (!writeToSegment(*m_localBufferSegment, m_sizeUsed, size, data))
        {
            Logging::error(__FILE__, __LINE__, "BufferWriterBW failed to write to segment");
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
            memcpy((void *)((char *)m_internalBuffer->bufferPtr + m_internalBuffer->offset), data, size);
        }
        else
        {
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
        // m_segmentHeader->counter = m_sizeUsed;
        return m_rdmaClient->writeRC(m_handle->node_id, remoteSegOffset, (void *)m_segmentHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true);
    }

    inline bool __attribute__((always_inline)) readHeaderFromRemote(size_t remoteSegOffset)
    {
        if (!m_rdmaClient->read(m_handle->node_id, remoteSegOffset, m_segmentHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true))
        {
            Logging::error(__FILE__, __LINE__, "BufferWriterBW tried to remotely read the next segment, but failed");
            return false;
        }
        return true;
    }

            
    bool getNextSegment(BufferSegment& newSegment_ret)
    {
        auto nextSegmentOffset = m_segmentHeader->nextSegmentOffset;

        Logging::debug(__FILE__, __LINE__, "BufferWriterBW getting new segment (reuseSegments = true)");
        //Set CanConsume flag on old segment
        Config::DPI_SEGMENT_HEADER_FLAGS::setCanConsumeSegment(m_segmentHeader->segmentFlags);
        Config::DPI_SEGMENT_HEADER_FLAGS::setCanWriteToSegment(m_segmentHeader->segmentFlags, false);
        m_segmentHeader->counter = m_sizeUsed;
        writeHeaderToRemote(m_localBufferSegment->offset);

        //Update m_segmentHeader to header of next segment
        m_rdmaClient->read(m_handle->node_id, nextSegmentOffset, m_segmentHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true);
        
        //Check if segment is free - todo: need a timeout??
        while (!Config::DPI_SEGMENT_HEADER_FLAGS::getCanWriteToSegment(m_segmentHeader->segmentFlags))
        {
            usleep(100);
            m_rdmaClient->read(m_handle->node_id, nextSegmentOffset, m_segmentHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true);
        }

        //Update segment with new data            
        newSegment_ret.offset = nextSegmentOffset;
        newSegment_ret.size = m_handle->segmentSizes;
        newSegment_ret.nextSegmentOffset = m_segmentHeader->nextSegmentOffset;

        return true;
    
    }

    BufferSegment *m_localBufferSegment = nullptr;
    size_t m_sizeUsed = 0;
    InternalBuffer *m_internalBuffer = nullptr;
    BufferHandle *m_handle = nullptr;
    RegistryClient *m_regClient = nullptr;
    RDMAClient *m_rdmaClient = nullptr; //
    Config::DPI_SEGMENT_HEADER_t* m_segmentHeader;
    bool deleteRdmaClient = false;

};

} // namespace dpi