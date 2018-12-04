#include "RegistryServer.h"

RegistryServer::RegistryServer() : ProtoServer("Registry Server", Config::DPI_REGISTRY_PORT)
{
    m_rdmaClient = new RDMAClient();
    for(size_t i=0; i<Config::DPI_NODES.size(); ++i){
        m_rdmaClient->connect(Config::DPI_NODES[i], i+1);
    }
    segmentHeaderBuffer = (Config::DPI_SEGMENT_HEADER_t*) m_rdmaClient->localAlloc(sizeof(Config::DPI_SEGMENT_HEADER_t));
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
    if (anyReq->Is<DPICreateRingOnBufferRequest>())
    {
        DPICreateRingOnBufferRequest createRingReq;
        DPICreateRingOnBufferResponse createRingResp;
        anyReq->UnpackTo(&createRingReq);

        string name = createRingReq.name();
        BufferHandle *buffHandle = retrieveBuffer(name);
        
        if (buffHandle == nullptr)
        {
            createRingResp.set_return_(MessageErrors::DPI_CREATE_RING_ON_BUF_FAILED);
        }
        else
        {
            BufferSegment* segment = createRingOnBuffer(buffHandle);
            createRingResp.set_name(name);
            createRingResp.set_node_id(buffHandle->node_id);
            DPICreateRingOnBufferResponse_Segment *seg = createRingResp.mutable_segment();
            seg->set_size(segment->size);
            seg->set_offset(segment->offset);
            seg->set_nextsegmentoffset(segment->nextSegmentOffset);
            createRingResp.set_segmentsperwriter(buffHandle->segmentsPerWriter);
            createRingResp.set_reusesegments(buffHandle->reuseSegments);
            createRingResp.set_segmentsizes(buffHandle->segmentSizes);
            createRingResp.set_return_(MessageErrors::NO_ERROR);
            delete segment;
        }

        anyResp->PackFrom(createRingResp);
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
            for (BufferSegment BufferSegment : buffHandle->entrySegments)
            {
                DPIRetrieveBufferResponse_Segment *segmentResp = retrieveBuffResp.add_segment();
                segmentResp->set_offset(BufferSegment.offset);
                segmentResp->set_size(BufferSegment.size);
                segmentResp->set_nextsegmentoffset(BufferSegment.nextSegmentOffset);
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
        size_t segmentsPerWriter = appendBuffReq.segmentsperwriter();
        bool reuseSegments = appendBuffReq.reusesegments();
        size_t segmentSizes = appendBuffReq.segmentsizes();
        if (appendBuffReq.register_())
        {
            BufferHandle buffHandle(name, appendBuffReq.node_id(), segmentsPerWriter, reuseSegments, segmentSizes);
            registerSuccess = registerBuffer(&buffHandle);
            if (registerSuccess)
            {
                appendBuffResp.set_return_(MessageErrors::NO_ERROR);
            }
            else
            {
                appendBuffResp.set_return_(MessageErrors::DPI_APPEND_BUFFHANDLE_FAILED);
            }
            anyResp->PackFrom(appendBuffResp);
            return;
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

        if (appendSuccess)
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

BufferSegment *RegistryServer::createRingOnBuffer(BufferHandle *bufferHandle)
{
    if (bufferHandle->node_id > Config::DPI_NODES.size())
    {
        return nullptr;
    }

    size_t offset = 0;
    string connection = Config::getIPFromNodeId(bufferHandle->node_id);
    size_t fullSegmentSize = bufferHandle->segmentSizes + sizeof(Config::DPI_SEGMENT_HEADER_t);

    if (!m_rdmaClient->remoteAlloc(connection, bufferHandle->segmentsPerWriter * fullSegmentSize, offset))
    {
        return nullptr;
    }

    //Write headers to allocated segments
    for (size_t i = 0; i < bufferHandle->segmentsPerWriter; i++)
    {
        bool lastSegment = i == bufferHandle->segmentsPerWriter-1;
        size_t nextSegOffset;
        if (lastSegment)
            nextSegOffset = (bufferHandle->reuseSegments ? offset : SIZE_MAX); //If reuseSegments is true, point last segment back to first (creating a ring), if not set it to max value
        else
            nextSegOffset = offset + (i+1) * fullSegmentSize;
        
        segmentHeaderBuffer->counter = 0;
        segmentHeaderBuffer->hasFollowSegment = (lastSegment && !bufferHandle->reuseSegments ? 0 : 1);
        segmentHeaderBuffer->nextSegmentOffset = nextSegOffset;
        segmentHeaderBuffer->segmentFlags = 0;

        if (!m_rdmaClient->writeRC(bufferHandle->node_id, offset + i * fullSegmentSize, segmentHeaderBuffer, sizeof(*segmentHeaderBuffer), true))
        {
            Logging::error(__FILE__, __LINE__, "RegistryServer: Error occured when writing header to newly created segments on buffer " + bufferHandle->name);
            return nullptr;
        }
    }

    Logging::debug(__FILE__, __LINE__, "Created ring with offset on entry segment: " + to_string(offset));

    auto bufferSegment = new BufferSegment(offset, bufferHandle->segmentSizes, bufferHandle->segmentSizes + sizeof(Config::DPI_SEGMENT_HEADER_t));
    return bufferSegment;
}

BufferHandle *RegistryServer::retrieveBuffer(string &name)
{
    if (m_bufferHandles.find(name) == m_bufferHandles.end())
    {
        Logging::error(__FILE__, __LINE__, "RegistryServer: Could not find Buffer Handle for buffer " + name);
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
    m_bufferHandles[name].entrySegments.push_back(*segment);
    DPI_DEBUG("Registry Server: Appended new segment to %s \n", name.c_str());
    DPI_DEBUG("Segment offset: %zu, size: %zu \n", (*segment).offset, (*segment).size);
    return true;
};