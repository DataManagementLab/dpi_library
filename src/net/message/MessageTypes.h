

#ifndef MESSAGETYPES_H_
#define MESSAGETYPES_H_

#include "../../utils/Config.h"

#include "HelloMessage.pb.h"
#include "RDMAConnRequest.pb.h"
#include "RDMAConnResponse.pb.h"
#include "RDMAConnRequestMgmt.pb.h"
#include "RDMAConnResponseMgmt.pb.h"
#include "MemoryResourceRequest.pb.h"
#include "MemoryResourceResponse.pb.h"
#include "DPICreateRingOnBufferRequest.pb.h"
#include "DPICreateRingOnBufferResponse.pb.h"
#include "DPIRetrieveBufferRequest.pb.h"
#include "DPIRetrieveBufferResponse.pb.h"
#include "DPIAppendBufferRequest.pb.h"
#include "DPIAppendBufferResponse.pb.h"
#include "DPIAllocSegmentsRequest.pb.h"
#include "DPIAllocSegmentsResponse.pb.h"

#include "ErrorMessage.pb.h"


#include <google/protobuf/any.pb.h>
#include <google/protobuf/message.h>
using google::protobuf::Any;

namespace dpi
{

enum MessageTypesEnum : int
{
  MEMORY_RESOURCE_REQUEST,
  MEMORY_RESOURCE_RELEASE,
};

class MessageTypes
{
public:


  static Any createDPIAllocSegmentsRequest(const string& bufferName, const size_t segmentsCount, const size_t segmentsSize, const bool reuseSegments, const bool newRing)
  {
    DPIAllocSegmentsRequest allocSegReq;
    allocSegReq.set_name(bufferName);
    allocSegReq.set_segments_count(segmentsCount);
    allocSegReq.set_segments_size(segmentsSize);
    allocSegReq.set_reuse_segments(reuseSegments);
    allocSegReq.set_new_ring(newRing);
    Any anyMessage;
    anyMessage.PackFrom(allocSegReq);
    return anyMessage;
  }

  static Any createDPICreateRingOnBufferRequest(string &name)
  {
    DPICreateRingOnBufferRequest createRingReq;
    createRingReq.set_name(name);
    Any anyMessage;
    anyMessage.PackFrom(createRingReq);
    return anyMessage;
  }

  static Any createDPIAppendBufferRequest(string &name, size_t offset, size_t size, size_t threshold)
  {
    DPIAppendBufferRequest appendBufferReq;
    appendBufferReq.set_name(name);
    appendBufferReq.set_register_(false);

    DPIAppendBufferRequest_Segment *segmentReq = appendBufferReq.add_segment();
    segmentReq->set_offset(offset);
    segmentReq->set_size(size);
    segmentReq->set_threshold(threshold);
    Any anyMessage;
    anyMessage.PackFrom(appendBufferReq);
    return anyMessage;
  }

  static Any createDPIRegisterBufferRequest(string &name, NodeID nodeId, size_t segmentsPerWriter, bool reuseSegments, size_t segmentSizes)
  {
    DPIAppendBufferRequest appendBufferReq;
    appendBufferReq.set_name(name);
    appendBufferReq.set_node_id(nodeId);
    appendBufferReq.set_segmentsperwriter(segmentsPerWriter);
    appendBufferReq.set_reusesegments(reuseSegments);
    appendBufferReq.set_segmentsizes(segmentSizes);
    appendBufferReq.set_register_(true);

    // DPIAppendBufferRequest_Segment *segmentReq = appendBufferReq.add_segment();
    Any anyMessage;
    anyMessage.PackFrom(appendBufferReq);
    return anyMessage;
  }

  static Any createDPIRetrieveBufferRequest(string &name)
  {
    DPIRetrieveBufferRequest retrieveBufferReq;
    retrieveBufferReq.set_name(name);

    Any anyMessage;
    anyMessage.PackFrom(retrieveBufferReq);
    return anyMessage;
  }

  static Any createMemoryResourceRequest(size_t size)
  {
    MemoryResourceRequest resReq;
    resReq.set_size(size);
    resReq.set_type(MessageTypesEnum::MEMORY_RESOURCE_REQUEST);
    Any anyMessage;
    anyMessage.PackFrom(resReq);
    return anyMessage;
  }

  static Any createMemoryResourceRequest(size_t size, string &name,
                                         bool persistent)
  {
    MemoryResourceRequest resReq;
    resReq.set_size(size);
    resReq.set_type(MessageTypesEnum::MEMORY_RESOURCE_REQUEST);
    resReq.set_name(name);
    resReq.set_persistent(persistent);

    Any anyMessage;
    anyMessage.PackFrom(resReq);
    return anyMessage;
  }

  static Any createMemoryResourceRelease(size_t size, size_t offset)
  {
    MemoryResourceRequest resReq;
    resReq.set_size(size);
    resReq.set_offset(offset);
    resReq.set_type(MessageTypesEnum::MEMORY_RESOURCE_RELEASE);
    Any anyMessage;
    anyMessage.PackFrom(resReq);
    return anyMessage;
  }
};
// end class
} // end namespace dpi

#endif /* MESSAGETYPES_H_ */
