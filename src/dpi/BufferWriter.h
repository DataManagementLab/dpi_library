/**
 * @brief BufferWriter implements two different RDMA strategies to write into the remote memory
 * 
 * @file BufferWriter.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"
#include "../net/rdma/RDMAClient.h"
#include "RegistryClient.h"
#include "BuffHandle.h"

namespace dpi
{
template <class base>
class BufferWriter : public base
{

  public:
    BufferWriter(BuffHandle *handle, size_t scratchPadSize = Config::DPI_SCRATCH_PAD_SIZE, RegistryClient* regClient = nullptr)
        : base(handle, scratchPadSize, regClient){};
    ~BufferWriter(){};

    //Append without use of scratchpad. Bad performance!
    bool append(void *data, size_t size)
    {
        while (size > this->m_scratchPadSize)
        {
            // update size move pointer
            size = size - this->m_scratchPadSize;
            memcpy(this->m_scratchPad, data, this->m_scratchPadSize);
            data = ((char *)data + this->m_scratchPadSize);
            if (!this->super_append(this->m_scratchPadSize))
                return false;
        }

        memcpy(this->m_scratchPad, data, size);
        return this->super_append(size);
    }

    //Append with scratchpad
    bool appendFromScratchpad(size_t size)
    {
        if (size > this->m_scratchPadSize)
        {
            std::cerr << "The size specified exceeds size of scratch pad size" << std::endl;
            return false;
        }

        return this->super_append(size);
    }

    void *getScratchPad()
    {
        return this->super_getScratchPad();
    }
};

class BufferWriterShared
{

  public:
    BufferWriterShared(BuffHandle *handle, size_t scratchPadSize, RegistryClient* regClient = nullptr) : m_handle(handle), m_scratchPadSize(scratchPadSize), m_regClient(regClient)
    {
        m_rdmaClient = new RDMAClient();
        m_rdmaClient->connect(handle->connection, handle->node_id);
        
        TO_BE_IMPLEMENTED(if(regClient == nullptr){
                          m_regClient = new RegistryClient();
                          }
                          m_regClient->connect(Config::DPI_REGISTRY_SERVER));
        m_scratchPad = m_rdmaClient->localAlloc(m_scratchPadSize);
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
        // get head handle check counter and compare to size / treshhold
        // if counter > tresh
            // request_buffer
            // check if actual new retrieved do again
        // if counter < tresh && size + counter > tresh && size fits into seg
            // update counter by size on remote f&a
            // create new seg
            // append seg to buffer

        m_rdmaClient->write(m_handle->node_id, 0, m_scratchPad, size, true);
        return true;
    }

    void *super_getScratchPad()
    {
        return m_scratchPad;
    }

  protected:
    BuffHandle *m_handle = nullptr;
    void *m_scratchPad = nullptr;
    size_t m_scratchPadSize = 0;

  private:
    // Scratch Pad
    RegistryClient *m_regClient = nullptr;
    RDMAClient *m_rdmaClient = nullptr; // used to issue RDMA req. to the NodeServer
};

class BufferWriterPrivate
{

  public:
    BufferWriterPrivate(BuffHandle *handle, size_t scratchPadSize, RegistryClient* regClient = nullptr) : m_handle(handle), m_scratchPadSize(scratchPadSize), m_regClient(regClient)
    {
        m_rdmaClient = new RDMAClient();
        m_rdmaClient->connect(handle->connection, handle->node_id);
        TO_BE_IMPLEMENTED(if(m_regClient == nullptr){
                          m_regClient = new RegistryClient();
                          }
                          m_regClient->connect(Config::DPI_REGISTRY_SERVER));
        m_scratchPad = m_rdmaClient->localAlloc(m_scratchPadSize);
    };

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

    bool super_append(size_t size)
    {

        if (m_localBufferSegments.empty() || m_localBufferSegments.back().size - m_sizeUsed < size)
        {
            size_t remoteOffset = 0;
            if (!m_rdmaClient->remoteAlloc(m_handle->connection, Config::DPI_SEGMENT_SIZE, remoteOffset))
            {
                return false;
            }
            m_sizeUsed = sizeof(Config::DPI_SEGMENT_HEADER_t);
            m_localBufferSegments.emplace_back(remoteOffset, Config::DPI_SEGMENT_SIZE, 0);
            m_regClient->dpi_append_segment(m_handle->name, m_localBufferSegments.back());
        }
        BuffSegment &segment = m_localBufferSegments.back();

        if (!m_rdmaClient->write(m_handle->node_id, segment.offset + m_sizeUsed, m_scratchPad, size, true))
        {
            return false;
        }
        m_sizeUsed = m_sizeUsed + size;
        // update counter / header once in a while
        TO_BE_IMPLEMENTED(m_rdmaClient->write(HEADER));
        return true;
    }

    void *super_getScratchPad()
    {
        return m_scratchPad;
    }

  protected:
    BuffHandle *m_handle = nullptr;
    vector<BuffSegment> m_localBufferSegments;
    void *m_scratchPad = nullptr;
    size_t m_scratchPadSize = 0;

  private:
    size_t m_sizeUsed = 0;
    Config::DPI_SEGMENT_HEADER_t HEADER;
    RegistryClient *m_regClient = nullptr;
    RDMAClient *m_rdmaClient = nullptr; // used to issue RDMA req. to the NodeServer
};

} // namespace dpi