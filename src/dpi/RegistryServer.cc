#include "RegistryServer.h"

RegistryServer::RegistryServer() : ProtoServer("Registry Server", Config::DPI_REGISTRY_PORT)
{
    m_rdmaClient = new RDMAClient();
    for(size_t i=0; i<Config::DPI_NODES.size(); ++i){
        m_rdmaClient->connect(Config::DPI_NODES[i]);
    }
};

RegistryServer::~RegistryServer()
{
    if(m_rdmaClient!=nullptr){
        delete m_rdmaClient;
        m_rdmaClient=nullptr;
    }
};

void RegistryServer::handle(Any *anyReq, Any *anyResp)
{
    if (anyReq->Is<DPICreateBufferRequest>())
    {
        DPICreateBufferRequest createBuffReq;
        DPICreateBufferResponse createBuffResp;
        anyReq->UnpackTo(&createBuffReq);

        string name = createBuffReq.name();
        BufferHandle *buffHandle = createBuffer(name, createBuffReq.node_id(), createBuffReq.size(), createBuffReq.threshold());
        
        if (buffHandle == nullptr)
        {
            createBuffResp.set_return_(MessageErrors::DPI_CREATE_BUFFHANDLE_FAILED);
        }
        else
        {
            createBuffResp.set_name(name);
            createBuffResp.set_node_id(buffHandle->node_id);
            for (BufferSegment BufferSegment : buffHandle->segments)
            {
                DPICreateBufferResponse_Segment *segmentResp = createBuffResp.add_segment();
                segmentResp->set_offset(BufferSegment.offset);
                segmentResp->set_size(BufferSegment.size);
                segmentResp->set_threshold(BufferSegment.threshold);
            }
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
        BufferHandle *buffHandle = retrieveBuffer(name);
        if (buffHandle == nullptr)
        {
            retrieveBuffResp.set_return_(MessageErrors::DPI_RTRV_BUFFHANDLE_FAILED);
        }
        else
        {
            retrieveBuffResp.set_name(name);
            retrieveBuffResp.set_node_id(buffHandle->node_id);
            for (BufferSegment BufferSegment : buffHandle->segments)
            {
                DPIRetrieveBufferResponse_Segment *segmentResp = retrieveBuffResp.add_segment();
                segmentResp->set_offset(BufferSegment.offset);
                segmentResp->set_size(BufferSegment.size);
                segmentResp->set_threshold(BufferSegment.threshold);
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

        bool registerSuccess = true;
        string name = appendBuffReq.name();
        if (appendBuffReq.register_())
        {
            BufferHandle buffHandle(name, appendBuffReq.node_id());
            registerSuccess = registerBuffer(&buffHandle);
        }

        bool appendSuccess = true;
        if (registerSuccess)
        {
            for (int64_t i = 0; i < appendBuffReq.segment_size(); ++i)
            {
                DPIAppendBufferRequest_Segment segmentReq = appendBuffReq.segment(i);
                BufferSegment segment(segmentReq.offset(), segmentReq.size(), segmentReq.threshold());
                if (!appendSegment(name, &segment))
                {
                    appendSuccess = false;
                    break;
                }
            }
        }

        if (registerSuccess && appendSuccess)
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

bool RegistryServer::registerBuffer(BufferHandle *buffHandle)
{
    if (m_bufferHandles.find(buffHandle->name) != m_bufferHandles.end())
    {
        return false;
    }
    m_bufferHandles[buffHandle->name] = *buffHandle;
    return true;
}

BufferHandle *RegistryServer::createBuffer(string &name, NodeID node_id, size_t size, size_t threshold)
{
    if (node_id > Config::DPI_NODES.size())
    {
        return nullptr;
    }

    if (m_bufferHandles.find(name) != m_bufferHandles.end())
    {
        return nullptr;
    }

    size_t offset = 0;
    string connection = Config::getIPFromNodeId(node_id);
    std::cout << "Connection " << connection << '\n';
    if (!m_rdmaClient->remoteAlloc(connection, size + sizeof(Config::DPI_SEGMENT_HEADER_t), offset))
    {
        return nullptr;
    }

    BufferHandle buffHandle(name, node_id);
    BufferSegment BufferSegment(offset, size, threshold); //lthostrup: todo encapsulate size of header into segment
    buffHandle.segments.push_back(BufferSegment);
    m_bufferHandles[name] = buffHandle;
    return &m_bufferHandles[name];
}

BufferHandle *RegistryServer::retrieveBuffer(string &name)
{
    if (m_bufferHandles.find(name) == m_bufferHandles.end())
    {
        return nullptr;
    }
    return &m_bufferHandles[name];
}

bool RegistryServer::appendSegment(string &name, BufferSegment *segment)
{
    if (m_bufferHandles.find(name) == m_bufferHandles.end())
    {
        return false;
    }
    m_bufferHandles[name].segments.push_back(*segment);
    DPI_DEBUG("Registry Server: Appended new segment to %s \n", name.c_str());
    DPI_DEBUG("Segment offset: %zu, size: %zu \n", (*segment).offset, (*segment).size);
    return true;
};