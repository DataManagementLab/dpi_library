

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
#include "LoadDataRequest.pb.h"
#include "LoadDataResponse.pb.h"
#include "DPICreateBufferRequest.pb.h"
#include "DPICreateBufferResponse.pb.h"
#include "DPIRetrieveBufferRequest.pb.h"
#include "DPIRetrieveBufferResponse.pb.h"
#include "DPIAppendBufferRequest.pb.h"
#include "DPIAppendBufferResponse.pb.h"

#include "ErrorMessage.pb.h"
#include "RegistrateReq.pb.h"
#include "RegistrateResp.pb.h"

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
  static Any createMemoryResourceRequest(size_t size)
  {
    MemoryResourceRequest resReq;
    resReq.set_size(size);
    resReq.set_type(MessageTypesEnum::MEMORY_RESOURCE_REQUEST);
    Any anyMessage;
    anyMessage.PackFrom(resReq);
    return anyMessage;
  }

  static Any createMemoryResourceRequest(const size_t size, const string &name,
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
}// end namespace dpi

#endif /* MESSAGETYPES_H_ */
