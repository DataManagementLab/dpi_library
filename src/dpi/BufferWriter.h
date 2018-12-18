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
public:
    virtual bool append(void *data, size_t size) = 0;
    virtual bool close() = 0;
};


class BufferWriterLat : public BufferWriter
{
   
  public:
    BufferWriterLat(string& bufferName, RegistryClient *regClient, size_t internalBufferSize = Config::DPI_INTERNAL_BUFFER_SIZE,  RDMAClient *rdmaClient = nullptr) : m_regClient(regClient), m_rdmaClient(rdmaClient)
    {   
        m_handle = regClient->joinBuffer(bufferName);
        
        if (m_handle->buffertype != BufferHandle::Buffertype::LAT)
        {
            Logging::error(__FILE__, __LINE__, "BufferWriterLat expects the buffer to be Buffertype::LAT)");
            return;
        }

        if (m_handle->entrySegments.size() != 1)
        {
            Logging::error(__FILE__, __LINE__, "BufferWriterLat expects only one entry segment in segment ring");
            return;
        }

        auto entrySegment = &m_handle->entrySegments.front();
        currentSegmentOffset = entrySegment->offset;

        Logging::debug(__FILE__, __LINE__, "BufferWriterLat ctor: offset on entry segment: " + to_string(currentSegmentOffset));

        if (m_rdmaClient == nullptr)
        {
            m_rdmaClient = new RDMAClient(internalBufferSize + sizeof(Config::DPI_SEGMENT_HEADER_t) + sizeof(uint64_t)); //Internal buffer, segmentHeader, consumedCnt
            m_rdmaClient->connect(Config::getIPFromNodeId(m_handle->node_id), m_handle->node_id);
            deleteRdmaClient = true;
        }

        m_segmentHeader = (Config::DPI_SEGMENT_HEADER_t *)m_rdmaClient->localAlloc(sizeof(Config::DPI_SEGMENT_HEADER_t));
        consumedCnt = (uint64_t *)m_rdmaClient->localAlloc(sizeof(uint64_t));

        //Read header
        readHeaderFromRemote(); 
        readCounterFromRemote();
        writeableFreeSegments = *consumedCnt;
        std::cout << "writeableFreeSegments: " << writeableFreeSegments << '\n';

        m_internalBuffer = new InternalBuffer(m_rdmaClient->localAlloc(internalBufferSize), internalBufferSize);
    }
    ~BufferWriterLat()
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
        m_segmentHeader->counter = m_handle->segmentSizes;
        m_segmentHeader->setWriteable(false);
        m_segmentHeader->setConsumable(true);
        m_segmentHeader->markEndSegment();
        std::cout << "marked end segment on offset: " << lastSegmentOffset << '\n';
        return writeHeaderToRemote(lastSegmentOffset);
    }

//Problem: the last append, will mark the segment as consumable, and the consumer will consume the segment and proceed onto the next segment, when closing the last segment will be marked as end, but the iterator already proceeded.
  private:

    bool super_append(void *data, size_t size)
    {
        if (writeableFreeSegments == 0)
        {
            std::cout << "No more segments to write to, reading updated remote counter. curSegOffset: " << currentSegmentOffset << " consumedCnt: " << *consumedCnt << '\n';
            uint64_t consumedCntOld = *consumedCnt;
            while (consumedCntOld == *consumedCnt)
            {
                readCounterFromRemote();
            }
            if (*consumedCnt < consumedCntOld) //uint64_t rolled over
            {
                Logging::fatal(__FILE__, __LINE__, "Consumed segments counter rolled over!");
            }
            writeableFreeSegments += *consumedCnt - consumedCntOld;
            std::cout << "New writeableFreeSegments: " << writeableFreeSegments << " - curSegOffset: " << currentSegmentOffset << '\n';
        }

        if (!writeToSegment(size, data, (writeableFreeSegments == 1 ? true : false)))
        {
            Logging::error(__FILE__, __LINE__, "BufferWriterBW failed to write to segment");
            return false;
        }

        --writeableFreeSegments;
        lastSegmentOffset = currentSegmentOffset;
        currentSegmentOffset = m_segmentHeader->nextSegmentOffset;
        readHeaderFromRemote(); //todo: calculate the next offset instead of reading it from header

        return true;
    }

    inline bool __attribute__((always_inline)) writeToSegment(size_t size, void *data, bool signaled = false)
    {
        //Update header, that will be written along with msg
        m_segmentHeader->counter = size;
        m_segmentHeader->setConsumable(true);

        //Increase size to also contain the header
        size += sizeof(Config::DPI_SEGMENT_HEADER_t); //Also include the header in the write

        if (size + m_internalBuffer->offset > m_internalBuffer->size)
        {
            // writeHeaderToRemote(); //Writing to the header is signaled, i.e. call only returns when all prior writes are sent
            m_internalBuffer->offset = 0;
        }
        memcpy((void *)((char *)m_internalBuffer->bufferPtr + m_internalBuffer->offset), m_segmentHeader, sizeof(Config::DPI_SEGMENT_HEADER_t));
        memcpy((void *)((char *)m_internalBuffer->bufferPtr + m_internalBuffer->offset + sizeof(Config::DPI_SEGMENT_HEADER_t)), data, size);

        void *bufferTmp = (void *)((char *)m_internalBuffer->bufferPtr + m_internalBuffer->offset);
        m_internalBuffer->offset += size;

        return m_rdmaClient->writeRC(m_handle->node_id, currentSegmentOffset, bufferTmp, size, signaled);
    }

    inline bool __attribute__((always_inline)) writeHeaderToRemote(size_t offset)
    {
        return m_rdmaClient->writeRC(m_handle->node_id, offset, (void *)m_segmentHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true);
    }

    inline bool __attribute__((always_inline)) readHeaderFromRemote()
    {
        if (!m_rdmaClient->read(m_handle->node_id, currentSegmentOffset, m_segmentHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true))
        {
            Logging::error(__FILE__, __LINE__, "BufferWriterBW tried to remotely read the next segment, but failed");
            return false;
        }
        return true;
    }

    inline bool __attribute__((always_inline)) readCounterFromRemote()
    {
        if (!m_rdmaClient->read(m_handle->node_id, m_handle->entrySegments[0].offset - sizeof(uint64_t), consumedCnt, sizeof(uint64_t), true)) //Counter is located just before the entrySegment
        {
            Logging::error(__FILE__, __LINE__, "BufferWriterBW tried to remotely read the consumed segments counter, but failed");
            return false;
        }
        // std::cout << "Reading counter from remote. offset: " << m_handle->entrySegments[0].offset - sizeof(uint64_t) << " value: " << *consumedCnt << '\n';
        return true;
    }

    size_t m_sizeUsed = 0;
    InternalBuffer *m_internalBuffer = nullptr;
    BufferHandle *m_handle = nullptr;
    RegistryClient *m_regClient = nullptr;
    RDMAClient *m_rdmaClient = nullptr; //
    Config::DPI_SEGMENT_HEADER_t* m_segmentHeader;
    bool deleteRdmaClient = false;
    size_t currentSegmentOffset = 0;
    size_t lastSegmentOffset = 0;
    uint64_t *consumedCnt = 0;
    uint64_t writeableFreeSegments = 0;
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
        m_segmentHeader->setWriteable(false);
        m_segmentHeader->setConsumable(true);
        m_segmentHeader->markEndSegment();
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
            //The data does not fit into segment, split it up --> write first part --> write the second part
            size_t firstPartSize = m_localBufferSegment->size - m_sizeUsed;
            if (firstPartSize > 0)
            {
                if (!writeToSegment(*m_localBufferSegment, m_sizeUsed, firstPartSize, data))
                {
                    return false;
                }
            }
            m_sizeUsed = m_sizeUsed + firstPartSize;
            getNextSegment();
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

            
    bool getNextSegment()
    {
        auto nextSegmentOffset = m_segmentHeader->nextSegmentOffset;
        Logging::debug(__FILE__, __LINE__, "BufferWriterBW getting new segment (reuseSegments = true)");
        //Set CanConsume flag on old segment
        m_segmentHeader->setConsumable(true);
        m_segmentHeader->setWriteable(false);
        m_segmentHeader->counter = m_sizeUsed;
        writeHeaderToRemote(m_localBufferSegment->offset);

        //Update m_segmentHeader to header of next segment
        m_rdmaClient->read(m_handle->node_id, nextSegmentOffset, m_segmentHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true);

        // std::cout << "New segment header, counter: " << m_segmentHeader->counter << ", nextSegOffset: " << m_segmentHeader->nextSegmentOffset << " flags: " << m_segmentHeader->segmentFlags << '\n';
        
        //Check if segment is free - todo: need a timeout??
        while (!m_segmentHeader->isWriteable())
        {
            usleep(100);
            m_rdmaClient->read(m_handle->node_id, nextSegmentOffset, m_segmentHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true);
        }

        //Update segment with new data            
        m_localBufferSegment->offset = nextSegmentOffset;
        // m_localBufferSegment->size = m_handle->segmentSizes;


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