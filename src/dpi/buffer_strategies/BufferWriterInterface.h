
#pragma once

#include "../../utils/Config.h"
#include "../../utils/Network.h"
#include "../../net/rdma/RDMAClient.h"
#include "../RegistryClient.h"
#include "../BuffHandle.h"


namespace dpi
{

class BufferWriterInterface
{
  public:
    BufferWriterInterface(BuffHandle *handle, size_t scratchPadSize, RegistryClient *regClient) : m_handle(handle), m_scratchPadSize(scratchPadSize), m_regClient(regClient)
    {
        m_rdmaClient = new RDMAClient();
        m_rdmaClient->connect(handle->connection, handle->node_id);
        setScratchpad(m_rdmaClient->localAlloc(m_scratchPadSize));

        TO_BE_IMPLEMENTED(if (regClient == nullptr) {
            m_regClient = new RegistryClient();
        } m_regClient->connect(Config::DPI_REGISTRY_SERVER));
    };

    virtual bool super_append(size_t size, size_t scratchPadOffset = 0) = 0;

  protected:
    void setScratchpad(void *scratchPad)
    {
        m_scratchPad = scratchPad;
    }

    bool allocRemoteSegment(BuffSegment& newSegment_ret)
    {
        size_t remoteOffset = 0;
        if (!m_rdmaClient->remoteAlloc(m_handle->connection, Config::DPI_SEGMENT_SIZE, remoteOffset))
        {
            return false;
        }
        newSegment_ret.offset = remoteOffset;
        newSegment_ret.size = Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t);
        newSegment_ret.threshold = Config::DPI_SEGMENT_SIZE * 0.8;
    
        m_regClient->dpi_append_segment(m_handle->name, newSegment_ret);
        return true;
    }

    bool writeToSegment(BuffSegment &segment, size_t offset, size_t size , size_t scratchPadOffset = 0)
    {
        return m_rdmaClient->write(m_handle->node_id, segment.offset + offset + sizeof(Config::DPI_SEGMENT_HEADER_t), m_scratchPad + scratchPadOffset, size, true);
    }

    BuffHandle *m_handle = nullptr;
    void *m_scratchPad = nullptr;
    size_t m_scratchPadSize = 0;
    RegistryClient *m_regClient = nullptr;
    RDMAClient *m_rdmaClient = nullptr; //
};

}