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
    Any sendAny = MessageTypes::createDPIRetrieveBufferRequest(name,false);

    BufferHandle *buffHandle = retrieveOrJoinBuffer(&sendAny, name);
    return buffHandle;
}



BufferHandle *RegistryClient::joinBuffer(string &name){
    Any sendAny = MessageTypes::createDPIRetrieveBufferRequest(name,true);
    BufferHandle *buffHandle = retrieveOrJoinBuffer(&sendAny, name);
    return buffHandle;

}


BufferHandle *RegistryClient::retrieveOrJoinBuffer(Any* sendAny, string &name){
        Any rcvAny;

    if (!this->send(sendAny, &rcvAny))
    {
        Logging::error(__FILE__, __LINE__, "Can not send message");
        return nullptr;
    }

    if (!rcvAny.Is<DPIRetrieveBufferResponse>())
    {
        Logging::error(__FILE__, __LINE__, "Unexpected response");
        return nullptr;
    }

    Logging::debug(__FILE__, __LINE__, "Received response");
    DPIRetrieveBufferResponse rtrvBufferResp;
    rcvAny.UnpackTo(&rtrvBufferResp);
    if (rtrvBufferResp.return_() != MessageErrors::NO_ERROR)
    {
        Logging::error(__FILE__, __LINE__, "Error on server side");
        return nullptr;
    }

    NodeID node_id = rtrvBufferResp.node_id();
    size_t segmentsPerWriter = rtrvBufferResp.segmentsperwriter();
    size_t segmentSizes = rtrvBufferResp.segmentsizes();
    size_t numberAppenders = rtrvBufferResp.segment_size();
    BufferHandle::Buffertype buffertype =  static_cast<BufferHandle::Buffertype>(rtrvBufferResp.buffertype());
    BufferHandle *buffHandle = new BufferHandle(name, node_id, segmentsPerWriter, numberAppenders, segmentSizes, buffertype);
    for (size_t i = 0; i < numberAppenders; ++i)
    {
        DPIRetrieveBufferResponse_Segment segmentResp = rtrvBufferResp.segment(i);
        BufferSegment segment(segmentResp.offset(), segmentResp.size());
        buffHandle->entrySegments.push_back(segment);
    }

    return buffHandle;
}

//Change so private can use this instead of create buffer
bool RegistryClient::registerBuffer(BufferHandle *handle)
{
    Any sendAny = MessageTypes::createDPIRegisterBufferRequest(*handle);
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
        Logging::error(__FILE__, __LINE__, "Can not send message");
        return false;
    }

    if (!rcvAny.Is<DPIAppendBufferResponse>())
    {
        Logging::error(__FILE__, __LINE__, "Unexpected response");
        return false;
    }

    Logging::debug(__FILE__, __LINE__, "Received response");
    DPIAppendBufferResponse appendBufferResp;
    rcvAny.UnpackTo(&appendBufferResp);
    if (appendBufferResp.return_() != MessageErrors::NO_ERROR)
    {
        Logging::error(__FILE__, __LINE__, "Error on server side");
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
    BufferHandle *buffHandle = new BufferHandle(name, node_id, segmentsPerWriter, 0, segmentSizes);

    DPICreateRingOnBufferResponse_Segment segmentResp = createRingResp.segment();
    BufferSegment segment(segmentResp.offset(), segmentResp.size());
    buffHandle->entrySegments.push_back(segment);

    return buffHandle;
}