/**
 * @brief 
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
    BufferWriter(BuffHandle *handle, size_t scratchPadSize = Config::DPI_SCRATCH_PAD_SIZE)
        : base(handle, scratchPadSize){};
    ~BufferWriter(){};

    bool append(void *data, size_t size)
    {
        return this->super_append(data, size);
    }
};

class BufferWriterShared
{

  public:
    BufferWriterShared(BuffHandle *handle, size_t scratchPadSize) : m_handle(handle), m_scratchPadSize(scratchPadSize)
    {
        m_rdmaClient = new RDMAClient();
        m_rdmaClient->connect(handle->connection, handle->node_id);
        m_scratchPad = m_rdmaClient->localAlloc(m_scratchPadSize);
    };
    ~BufferWriterShared()
    {
        delete m_rdmaClient;
    };
    bool super_append(void *data, size_t size)
    {
        if (size > m_scratchPadSize)
        {
            return false;
        }
        else
        {
            memcpy(m_scratchPad, data, size);
            m_rdmaClient->write(m_handle->node_id, 0, m_scratchPad, size, true);
            return true;
        }
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
    BufferWriterPrivate(BuffHandle *handle, size_t scratchPadSize) : m_handle(handle), m_scratchPadSize(scratchPadSize)
    {
        m_rdmaClient = new RDMAClient();
        m_rdmaClient->connect(handle->connection, handle->node_id);
        m_scratchPad = m_rdmaClient->localAlloc(m_scratchPadSize);
    };
    ~BufferWriterPrivate()
    {
        delete m_rdmaClient;
    };
    bool super_append(void *data, size_t size)
    {
        if (size > m_scratchPadSize)
        {
            return false;
        }
        else
        {    
            if(m_localBufferSegments.size() == 0 ){
                std::cout << "Allocating first Buffer" << ((int*)data)[0] << '\n';
                size_t remoteOffset = 0; 
                assert(m_rdmaClient->remoteAlloc(m_handle->connection, Config::DPI_SEGMENT_SIZE, remoteOffset));
                m_localBufferSegments.emplace_back(remoteOffset, Config::DPI_SEGMENT_SIZE,0,0);
            }
            auto& segment = m_localBufferSegments.back();

            if(segment.size - segment.sizeUsed < size){
                std::cout << "Allocating additional Buffer" << ((int*)data)[0] << '\n';
                size_t remoteOffset = 0; 
                assert(m_rdmaClient->remoteAlloc(m_handle->connection, Config::DPI_SEGMENT_SIZE, remoteOffset));
                m_localBufferSegments.emplace_back(remoteOffset, Config::DPI_SEGMENT_SIZE,0,0);
                segment= m_localBufferSegments.back();
            }
            
            memcpy(m_scratchPad, data, size);
            m_rdmaClient->write(m_handle->node_id, segment.offset + segment.sizeUsed, m_scratchPad, size, true);
            segment.sizeUsed += size;
            return true;
        }
        return false;
    }

  protected:
    BuffHandle *m_handle = nullptr;
    vector<BuffSegment> m_localBufferSegments;
    void *m_scratchPad = nullptr;
    size_t m_scratchPadSize = 0;

  private:
    // Scratch Pad

    RegistryClient *m_regClient = nullptr;
    RDMAClient *m_rdmaClient = nullptr; // used to issue RDMA req. to the NodeServer
};

} // namespace dpi