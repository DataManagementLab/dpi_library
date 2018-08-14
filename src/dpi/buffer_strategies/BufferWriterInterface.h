
#pragma once

#include "../../utils/Config.h"
#include "../../utils/Network.h"
#include "../../net/rdma/RDMAClient.h"
#include "../RegistryClient.h"
#include "../BufferHandle.h"


namespace dpi
{
struct InternalBuffer
{
    void* bufferPtr = nullptr;
    size_t size = 0;
    size_t offset = 0;
    InternalBuffer(void* bufferPtr, size_t size) : bufferPtr(bufferPtr), size(size){};
};
class BufferWriterInterface
{
  public:
    BufferWriterInterface(BufferHandle *handle, size_t internalBufferSize, RegistryClient *regClient) : m_handle(handle), m_regClient(regClient)
    {
        m_rdmaClient = new RDMAClient();
        m_rdmaClient->connect(Config::getIPFromNodeId(handle->node_id), handle->node_id);
        m_rdmaHeader = (Config::DPI_SEGMENT_HEADER_t *)m_rdmaClient->localAlloc(sizeof(Config::DPI_SEGMENT_HEADER_t));

        m_internalBuffer = new InternalBuffer(m_rdmaClient->localAlloc(internalBufferSize), internalBufferSize);
    };

    ~BufferWriterInterface()
    {
        if (m_internalBuffer != nullptr && m_internalBuffer->bufferPtr != nullptr)
        {
            m_rdmaClient->localFree(m_internalBuffer->bufferPtr);
        }
        delete m_rdmaClient;
    };

    virtual bool super_append(void *data, size_t size) = 0;

    virtual bool super_close() = 0;


  protected:

    // void setScratchpad(void *scratchPad) delete
    // {
    //     m_scratchPad = scratchPad;
    // }

    bool allocRemoteSegment(BufferSegment& newSegment_ret)
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

    
    // inline bool __attribute__((always_inline)) writeToSegment(BufferSegment &segment, size_t insideSegOffset, size_t size , void* data)
    // {
    //     // std::cout << "Writing to segment, size: " << size << " insideSegOffset: " << insideSegOffset << " circular buf size: " << m_internalBuffer->size << " size + m_internalBuffer->offset: " << size + m_internalBuffer->offset << " data: " << ((int*)data)[0] << '\n';
    //     if (size + m_internalBuffer->offset <= m_internalBuffer->size)
    //     {
    //         // std::cout << "Fit into circular buffer" << '\n';
    //         memcpy((void*)((char*)m_internalBuffer->bufferPtr + m_internalBuffer->offset), data, size);
    //     }
    //     else
    //     {
    //         // std::cout << "Does not fit into circular buffer, sending header first" << '\n';
    //         // writeHeaderToRemote(segment.offset); //Writing to the header is signalled, i.e. call only returns when all prior writes are sent
    //         memcpy(m_internalBuffer->bufferPtr, data, size);
    //         m_internalBuffer->offset = 0;
    //     }

    //     void* bufferTmp = (void*) ((char*)m_internalBuffer->bufferPtr + m_internalBuffer->offset);
    //     m_internalBuffer->offset += size;

    //     return m_rdmaClient->writeRC(m_handle->node_id, segment.offset + insideSegOffset + sizeof(Config::DPI_SEGMENT_HEADER_t), bufferTmp , size, false);
    // }

    // inline bool __attribute__((always_inline)) writeHeaderToRemote(size_t remoteSegOffset)
    // {
    //     return m_rdmaClient->writeRC(m_handle->node_id, remoteSegOffset , (void*)m_rdmaHeader, sizeof(Config::DPI_SEGMENT_HEADER_t), true);
    // }

    InternalBuffer *m_internalBuffer = nullptr;
    BufferHandle *m_handle = nullptr;
    RegistryClient *m_regClient = nullptr;
    RDMAClient *m_rdmaClient = nullptr; //
    Config::DPI_SEGMENT_HEADER_t* m_rdmaHeader;

};

}