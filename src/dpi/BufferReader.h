/**
 * @brief BufferReader implements a read method that pulls latest bufferhandle from the registry and reads the buffer data
 * 
 * @file BufferReader.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-13
 */

#pragma once

#include "../utils/Config.h"
#include "../utils/Network.h"
#include "../net/rdma/RDMAClient.h"
#include "RegistryClient.h"

#include "BufferHandle.h"

namespace dpi
{

class BufferReader
{
public:
    BufferReader(BufferHandle *handle, RegistryClient *regClient) : m_handle(handle), m_regClient(regClient)
    {
        m_rdmaClient = new RDMAClient();
        m_rdmaClient->connect(Config::getIPFromNodeId(handle->node_id), handle->node_id);
    };
    ~BufferReader()
    {
        delete m_rdmaClient;
    };

    void* read(size_t &sizeRead, bool withHeader = false)
    {   
        //Update buffer handle
        m_handle = m_regClient->retrieveBuffer(m_handle->name);
        Logging::debug(__FILE__, __LINE__, "Retrieved new BufferHandle");
        if (m_handle->entrySegments.size() == 0) 
        {
            Logging::error(__FILE__, __LINE__, "Buffer Handle did not contain any segments");
            return nullptr;
        }
        size_t dataSize = 0;
        for (BufferSegment &segment : m_handle->entrySegments)
        {
            dataSize += (segment.size + (withHeader ? sizeof(Config::DPI_SEGMENT_HEADER_t) : 0)) * m_handle->segmentsPerWriter;
        }
        void* data = m_rdmaClient->localAlloc(dataSize);
        Config::DPI_SEGMENT_HEADER_t* header = (Config::DPI_SEGMENT_HEADER_t*) m_rdmaClient->localAlloc(sizeof(Config::DPI_SEGMENT_HEADER_t));
        size_t writeoffset = 0;
        for (BufferSegment &segment : m_handle->entrySegments)
        {
            size_t firstSegmentOffset = segment.offset;
            size_t currentSegmentOffset = segment.offset;
            do
            {
                std::cout << "Reading header on offset: " << currentSegmentOffset << '\n';
                m_rdmaClient->read(m_handle->node_id, currentSegmentOffset, (void*)(header), sizeof(Config::DPI_SEGMENT_HEADER_t), true);
                size_t segmentDataSize = header->counter;
                
                //Read data from segment
                if (!withHeader)
                {
                    m_rdmaClient->read(m_handle->node_id, currentSegmentOffset + sizeof(Config::DPI_SEGMENT_HEADER_t), (void*)(((char*)data) + writeoffset), header->counter, true);
                    writeoffset += segmentDataSize;
                }
                else
                {
                    m_rdmaClient->read(m_handle->node_id, currentSegmentOffset, (void*)(((char*)data) + writeoffset), header->counter + sizeof(Config::DPI_SEGMENT_HEADER_t), true);
                    writeoffset += (segmentDataSize + sizeof(Config::DPI_SEGMENT_HEADER_t));
                }            
                Logging::debug(__FILE__, __LINE__, "Read data from segment");
                currentSegmentOffset = header->nextSegmentOffset;
            } while (!header->isEndSegment());

            //Read header of segment
            
        }
        sizeRead = writeoffset;
        return (void*) data;
    };

private:
    BufferHandle *m_handle = nullptr;
    RegistryClient *m_regClient = nullptr;
    RDMAClient *m_rdmaClient = nullptr;

};

} // namespace dpi