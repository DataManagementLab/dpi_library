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

BuffHandle *RegistryClient::dpi_create_buffer(string &name, NodeID node_id, size_t size, size_t threshold)
{
    Any sendAny = MessageTypes::createDPICreateBufferRequest(name, node_id, size, threshold);
    Any rcvAny;

    if (!this->send(&sendAny, &rcvAny))
    {
        Logging::fatal(__FILE__, __LINE__, "Can not send message");
        return nullptr;
    }

    if (!rcvAny.Is<DPICreateBufferResponse>())
    {
        Logging::fatal(__FILE__, __LINE__, "Unexpected response");
        return nullptr;
    }

    Logging::debug(__FILE__, __LINE__, "Received response");
    DPICreateBufferResponse createBufferResp;
    rcvAny.UnpackTo(&createBufferResp);
    if (createBufferResp.return_() != MessageErrors::NO_ERROR)
    {
        Logging::fatal(__FILE__, __LINE__, "Error on server side");
        return nullptr;
    }

    BuffHandle *buffHandle = new BuffHandle(name, node_id);
    for (size_t i = 0; i < createBufferResp.segment_size(); ++i)
    {
        DPICreateBufferResponse_Segment segmentResp = createBufferResp.segment(i);
        BuffSegment segment(segmentResp.offset(), segmentResp.size(), segmentResp.threshold());
        buffHandle->segments.push_back(segment);
    }

    return buffHandle;
}

BuffHandle *RegistryClient::dpi_retrieve_buffer(string &name)
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
    BuffHandle *buffHandle = new BuffHandle(name, node_id);
    for (size_t i = 0; i < rtrvBufferResp.segment_size(); ++i)
    {
        DPIRetrieveBufferResponse_Segment segmentResp = rtrvBufferResp.segment(i);
        BuffSegment segment(segmentResp.offset(), segmentResp.size(), segmentResp.threshold());
        buffHandle->segments.push_back(segment);
    }

    return buffHandle;
}

bool RegistryClient::dpi_register_buffer(BuffHandle *handle)
{
    if (handle->segments.size() != 1)
    {
        Logging::fatal(__FILE__, __LINE__, "Too many segments in handle");
        return false;
    }

    BuffSegment segment = handle->segments[0];
    Any sendAny = MessageTypes::createDPIRegisterBufferRequest(handle->name, handle->node_id, segment.offset, segment.size, segment.threshold);
    return dpi_append_or_retrieve_segment(&sendAny);
}

bool RegistryClient::dpi_append_segment(string &name, BuffSegment &segment)
{
    Any sendAny = MessageTypes::createDPIAppendBufferRequest(name, segment.offset, segment.size, segment.threshold);
    return dpi_append_or_retrieve_segment(&sendAny);
}

bool RegistryClient::dpi_append_or_retrieve_segment(Any *sendAny)
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