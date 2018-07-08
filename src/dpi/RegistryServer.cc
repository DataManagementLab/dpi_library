#include "RegistryServer.h"

RegistryServer::RegistryServer() : ProtoServer("Registry Server", Config::DPI_REGISTRY_PORT)
{
    m_rdmaClient = new RDMAClient();
};

RegistryServer::RegistryServer(int port) : ProtoServer("Registry Server", port)
{
    m_rdmaClient = new RDMAClient();
};

RegistryServer::~RegistryServer()
{
    delete m_rdmaClient;
};

void RegistryServer::handle(Any *anyReq, Any *anyResp)
{
    if (anyReq->Is<DPICreateBufferRequest>())
    {
        DPICreateBufferRequest createBuffReq;
        DPICreateBufferResponse createBuffResp;
        anyReq->UnpackTo(&createBuffReq);

        string name = createBuffReq.name();
        BuffHandle *buffHandle = dpi_create_buffer(name, createBuffReq.node_id(), createBuffReq.size(), createBuffReq.threshold());
        if (buffHandle == nullptr)
        {
            createBuffResp.set_return_(MessageErrors::DPI_CREATE_BUFFHANDLE_FAILED);
        }
        else
        {
            createBuffResp.set_return_(MessageErrors::NO_ERROR);
        }

        anyResp->PackFrom(createBuffResp);
    }
    else if (anyReq->Is<DPIRetrieveBufferRequest>())
    {
        DPIRetrieveBufferRequest retrieveBuffReq;
        DPIRetrieveBufferResponse retrieveBuffResp;
        anyReq->UnpackTo(&retrieveBuffReq);

        string name = retrieveBuffReq.name();
        BuffHandle *buffHandle = dpi_retrieve_buffer(name);
        if (buffHandle == nullptr)
        {
            retrieveBuffResp.set_return_(MessageErrors::DPI_RTRV_BUFFHANDLE_FAILED);
        }
        else
        {
            retrieveBuffResp.set_name(name);
            retrieveBuffResp.set_node_id(buffHandle->node_id);
            for (BuffSegment buffSegment : buffHandle->segments)
            {
                DPIRetrieveBufferResponse_Segment *segmentResp = retrieveBuffResp.add_segment();
                segmentResp->set_offset(buffSegment.offset);
                segmentResp->set_size(buffSegment.size);
                segmentResp->set_threshold(buffSegment.threshold);
            }
            retrieveBuffResp.set_return_(MessageErrors::NO_ERROR);
        }
        anyResp->PackFrom(retrieveBuffResp);
    }
    else if (anyReq->Is<DPIAppendBufferRequest>())
    {
        DPIAppendBufferRequest appendBuffReq;
        DPIAppendBufferResponse appendBuffResp;
        anyReq->UnpackTo(&appendBuffReq);

        string name = appendBuffReq.name();
        bool success = true;
        for (size_t i = 0; i < appendBuffReq.segment_size(); ++i)
        {
            DPIAppendBufferRequest_Segment segmentReq = appendBuffReq.segment(i);
            BuffSegment segment(segmentReq.offset(), segmentReq.size(), segmentReq.threshold());
            if (!dpi_append_segment(name, &segment))
            {
                success = false;
                break;
            }
        }

        if (success)
        {
            appendBuffResp.set_return_(MessageErrors::NO_ERROR);
        }
        else
        {
            appendBuffResp.set_return_(MessageErrors::DPI_APPEND_BUFFHANDLE_FAILED);
        }

        anyResp->PackFrom(appendBuffResp);
    }
    else
    {
        ErrorMessage errorResp;
        errorResp.set_return_(MessageErrors::INVALID_MESSAGE);
        anyResp->PackFrom(errorResp);
    }
}

bool RegistryServer::dpi_register_buffer(BuffHandle *buffHandle)
{
    if (m_bufferHandles.find(buffHandle->name) != m_bufferHandles.end())
    {
        return false;
    }
    m_bufferHandles[buffHandle->name] = *buffHandle;
    return true;
}

BuffHandle *RegistryServer::dpi_create_buffer(string &name, NodeID node_id, size_t size, size_t threshold)
{
    if (node_id >= Config::DPI_NODES.size())
    {
        return nullptr;
    }

    if (m_bufferHandles.find(name) != m_bufferHandles.end())
    {
        return nullptr;
    }

    size_t offset = 0;
    string connection = Config::DPI_NODES[node_id];
    if (!m_rdmaClient->remoteAlloc(connection, size, offset))
    {
        return nullptr;
    }

    BuffHandle buffHandle(name, node_id);
    BuffSegment buffSegment(offset, size, threshold);
    buffHandle.segments.push_back(buffSegment);
    m_bufferHandles[name] = buffHandle;
    return &m_bufferHandles[name];
}

BuffHandle *RegistryServer::dpi_retrieve_buffer(string &name)
{
    if (m_bufferHandles.find(name) == m_bufferHandles.end())
    {
        return nullptr;
    }
    return &m_bufferHandles[name];
}

bool RegistryServer::dpi_append_segment(string &name, BuffSegment *segment)
{
    //register buffer if it does not exist
    if (m_bufferHandles.find(name) == m_bufferHandles.end())
    {
        return false;
    }
    m_bufferHandles[name].segments.push_back(*segment);
    return true;
};