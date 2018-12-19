#include "RegistryServer.h"

RegistryServer::RegistryServer() : ProtoServer("Registry Server", Config::DPI_REGISTRY_PORT)
{
    m_rdmaClient = new RDMAClient();
    for (size_t i = 0; i < Config::DPI_NODES.size(); ++i)
    {
        m_rdmaClient->connect(Config::DPI_NODES[i], i + 1);
    }

    counterRdmaBuf = (uint64_t*)m_rdmaClient->localAlloc(sizeof(uint64_t));
};

RegistryServer::~RegistryServer()
{
    if (m_rdmaClient != nullptr)
    {
        delete m_rdmaClient;
        m_rdmaClient = nullptr;
    }
};

void RegistryServer::handle(Any *anyReq, Any *anyResp)
{
    if (anyReq->Is<DPICreateRingOnBufferRequest>())
    {
        Logging::debug(__FILE__, __LINE__, "Received DPICreateRingOnBufferRequest");
        DPICreateRingOnBufferRequest createRingReq;
        DPICreateRingOnBufferResponse createRingResp;
        anyReq->UnpackTo(&createRingReq);

        string name = createRingReq.name();
        Logging::debug(__FILE__, __LINE__, "Retrieving buffer");
        BufferHandle *buffHandle = retrieveBuffer(name);

        if (buffHandle == nullptr)
        {
            createRingResp.set_return_(MessageErrors::DPI_CREATE_RING_ON_BUF_FAILED);
        }
        else
        {
            BufferSegment *segment = createRingOnBuffer(buffHandle);
            createRingResp.set_name(name);
            createRingResp.set_node_id(buffHandle->node_id);
            DPICreateRingOnBufferResponse_Segment *seg = createRingResp.mutable_segment();
            seg->set_size(segment->size);
            seg->set_offset(segment->offset);
            createRingResp.set_segmentsperwriter(buffHandle->segmentsPerWriter);
            createRingResp.set_segmentsizes(buffHandle->segmentSizes);
            createRingResp.set_return_(MessageErrors::NO_ERROR);
            buffHandle->entrySegments.push_back(*segment);
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
        else if (retrieveBuffReq.isappender())
        {
            auto &numberAppenders = m_appendersJoinedBuffer[name];
            if (numberAppenders == buffHandle->entrySegments.size())
            {
                retrieveBuffResp.set_return_(MessageErrors::DPI_JOIN_BUFFER_FAILED);
                Logging::error(
                    __FILE__,
                    __LINE__,
                    "DPI Join Buffer failed, all rings are in use. Increase number of appenders in creation of Buffer ");
            }
            else
            {
                retrieveBuffResp.set_name(name);
                retrieveBuffResp.set_node_id(buffHandle->node_id);
                BufferSegment BufferSegment = buffHandle->entrySegments[numberAppenders];
                DPIRetrieveBufferResponse_Segment *segmentResp = retrieveBuffResp.add_segment();
                segmentResp->set_offset(BufferSegment.offset);
                segmentResp->set_size(BufferSegment.size);
                retrieveBuffResp.set_segmentsizes(buffHandle->segmentSizes);
                retrieveBuffResp.set_segmentsperwriter(buffHandle->segmentsPerWriter);
                retrieveBuffResp.set_buffertype(buffHandle->buffertype);
                ++numberAppenders;
                retrieveBuffResp.set_return_(MessageErrors::NO_ERROR);
            }
            
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
            }
            retrieveBuffResp.set_segmentsizes(buffHandle->segmentSizes);
            retrieveBuffResp.set_segmentsperwriter(buffHandle->segmentsPerWriter);
            retrieveBuffResp.set_buffertype(buffHandle->buffertype);
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
        size_t segmentSizes = appendBuffReq.segmentsizes();
        size_t numberAppenders = appendBuffReq.numberappenders();
        BufferHandle::Buffertype bufferType = static_cast<BufferHandle::Buffertype>(appendBuffReq.buffertype());

        if (appendBuffReq.register_())
        {
            BufferHandle buffHandle(name, appendBuffReq.node_id(), segmentsPerWriter, numberAppenders, segmentSizes, bufferType);
            registerSuccess = registerBuffer(&buffHandle);
            

            BufferHandle *buffHandlePtr = retrieveBuffer(name);
            if (registerSuccess)
            {
                // Creating as many rings as appenders
                for (size_t i = 0; i < numberAppenders; i++)
                {
                    std::cout << "Register Called: Creating new Ring" << '\n';
                    BufferSegment *segment = createRingOnBuffer(buffHandlePtr);
                    if (segment == nullptr)
                    {
                        registerSuccess = false;
                        break;
                    }

                    buffHandlePtr->entrySegments.push_back(*segment);
                }
            }
            

            m_appendersJoinedBuffer[name] = 0;

            if (registerSuccess)
            {
                appendBuffResp.set_return_(MessageErrors::NO_ERROR);
            }
            else
            {
                appendBuffResp.set_return_(MessageErrors::DPI_REGISTER_BUFFHANDLE_FAILED);
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
                BufferSegment segment(segmentReq.offset(), segmentReq.size());
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

    if (!m_rdmaClient->remoteAllocSegments(connection, bufferHandle->name, bufferHandle->segmentsPerWriter, fullSegmentSize, offset, bufferHandle->buffertype))
    {
        return nullptr;
    }

    Logging::debug(__FILE__, __LINE__, "Created ring with offset on entry segment: " + to_string(offset));

    auto bufferSegment = new BufferSegment(offset, bufferHandle->segmentSizes);
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