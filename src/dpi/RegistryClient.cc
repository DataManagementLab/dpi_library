#include "RegistryClient.h"

RegistryClient::RegistryClient()
    : ProtoClient(Config::DPI_REGISTRY_SERVER, Config::DPI_REGISTRY_PORT)
{
    if (!this->connect())
    {
        throw invalid_argument("Can not connect to DPI registry");
    }
}

RegistryClient::~RegistryClient()
{
}

// BufferHandle *RegistryClient::createBuffer(string &name, NodeID node_id, size_t size, size_t threshold)
// {
//     Any sendAny = MessageTypes::createDPICreateBufferRequest(name, node_id, size, threshold);
//     Any rcvAny;

//     if (!this->send(&sendAny, &rcvAny))
//     {
//         Logging::fatal(__FILE__, __LINE__, "Can not send message");
//         return nullptr;
//     }

//     if (!rcvAny.Is<DPICreateBufferResponse>())
//     {
//         Logging::fatal(__FILE__, __LINE__, "Unexpected response");
//         return nullptr;
//     }

//     Logging::debug(__FILE__, __LINE__, "Received response");
//     DPICreateBufferResponse createBufferResp;
//     rcvAny.UnpackTo(&createBufferResp);
//     if (createBufferResp.return_() != MessageErrors::NO_ERROR)
//     {
//         Logging::fatal(__FILE__, __LINE__, "Error on server side");
//         return nullptr;
//     }

//     BufferHandle *buffHandle = new BufferHandle(name, node_id);
//     for (int64_t i = 0; i < createBufferResp.segment_size(); ++i)
//     {
//         DPICreateBufferResponse_Segment segmentResp = createBufferResp.segment(i);
//         BufferSegment segment(segmentResp.offset(), segmentResp.size(), segmentResp.threshold());
//         buffHandle->segments.push_back(segment);
//     }

//     return buffHandle;
// }

BufferHandle *RegistryClient::retrieveBuffer(string &name)
{
    Any sendAny = MessageTypes::createDPIRetrieveBufferRequest(name);
    Any rcvAny;

    if (!this->send(&sendAny, &rcvAny))
    {
        Logging::fatal(__FILE__, __LINE__, "Can not send message");
        return nullptr;
    }

    if (!rcvAny.Is<DPIRetrieveBufferResponse>())
    {
        Logging::fatal(__FILE__, __LINE__, "Unexpected response");
        return nullptr;
    }

    Logging::debug(__FILE__, __LINE__, "Received response");
    DPIRetrieveBufferResponse rtrvBufferResp;
    rcvAny.UnpackTo(&rtrvBufferResp);
    if (rtrvBufferResp.return_() != MessageErrors::NO_ERROR)
    {
        Logging::fatal(__FILE__, __LINE__, "Error on server side");
        return nullptr;
    }

    NodeID node_id = rtrvBufferResp.node_id();
    size_t segmentsPerWriter = rtrvBufferResp.segmentsperwriter();
    bool reuseSegments = rtrvBufferResp.reusesegments();
    size_t segmentSizes = rtrvBufferResp.segmentsizes();
    BufferHandle *buffHandle = new BufferHandle(name, node_id, segmentsPerWriter, segmentSizes);
    for (int64_t i = 0; i < rtrvBufferResp.segment_size(); ++i)
    {
        DPIRetrieveBufferResponse_Segment segmentResp = rtrvBufferResp.segment(i);
        BufferSegment segment(segmentResp.offset(), segmentResp.size(), segmentResp.nextsegmentoffset());
        buffHandle->entrySegments.push_back(segment);
    }

    return buffHandle;
}

//Change so private can use this instead of create buffer
bool RegistryClient::registerBuffer(BufferHandle *handle)
{
    if (handle->entrySegments.size() != 0)
    {
        Logging::error(__FILE__, __LINE__, "Can only register handle without segments");
        return false;
    }

    Any sendAny = MessageTypes::createDPIRegisterBufferRequest(handle->name, handle->node_id, handle->segmentsPerWriter, handle->segmentSizes);
    return appendOrRetrieveSegment(&sendAny);
}

// bool RegistryClient::appendSegment(string &name, BufferSegment &segment)
// {
//     Any sendAny = MessageTypes::createDPIAppendBufferRequest(name, segment.offset, segment.size, segment.nextSegmentOffset);
//     return appendOrRetrieveSegment(&sendAny);
// }

bool RegistryClient::appendOrRetrieveSegment(Any *sendAny)
{
    Any rcvAny;
    if (!this->send(sendAny, &rcvAny))
    {
        Logging::fatal(__FILE__, __LINE__, "Can not send message");
        return false;
    }

    if (!rcvAny.Is<DPIAppendBufferResponse>())
    {
        Logging::fatal(__FILE__, __LINE__, "Unexpected response");
        return false;
    }

    Logging::debug(__FILE__, __LINE__, "Received response");
    DPIAppendBufferResponse appendBufferResp;
    rcvAny.UnpackTo(&appendBufferResp);
    if (appendBufferResp.return_() != MessageErrors::NO_ERROR)
    {
        Logging::fatal(__FILE__, __LINE__, "Error on server side");
        return false;
    }

    return true;
}

BufferHandle *RegistryClient::createSegmentRingOnBuffer(string &name)
{
    Any sendAny = MessageTypes::createDPICreateRingOnBufferRequest(name);
    Any rcvAny;
    if (!this->send(&sendAny, &rcvAny))
    {
        Logging::fatal(__FILE__, __LINE__, "Can not send message");
        return nullptr;
    }

    if (!rcvAny.Is<DPICreateRingOnBufferResponse>())
    {
        Logging::fatal(__FILE__, __LINE__, "Unexpected response");
        return nullptr;
    }

    Logging::debug(__FILE__, __LINE__, "Received response");
    DPICreateRingOnBufferResponse createRingResp;
    rcvAny.UnpackTo(&createRingResp);
    if (createRingResp.return_() != MessageErrors::NO_ERROR)
    {
        Logging::fatal(__FILE__, __LINE__, "Error on server side");
        return nullptr;
    }

    NodeID node_id = createRingResp.node_id();
    size_t segmentsPerWriter = createRingResp.segmentsperwriter();
    size_t segmentSizes = createRingResp.segmentsizes();
    BufferHandle *buffHandle = new BufferHandle(name, node_id, segmentsPerWriter, segmentSizes);

    DPICreateRingOnBufferResponse_Segment segmentResp = createRingResp.segment();
    BufferSegment segment(segmentResp.offset(), segmentResp.size(), segmentResp.nextsegmentoffset());
    buffHandle->entrySegments.push_back(segment);

    return buffHandle;
}