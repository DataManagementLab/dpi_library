

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

#include "ErrorMessage.pb.h"
#include "RegistrateReq.pb.h"
#include "RegistrateResp.pb.h"

#include  <google/protobuf/any.pb.h>
#include <google/protobuf/message.h>
using google::protobuf::Any;

namespace dpi {

enum MessageTypesEnum
  : int {
    MEMORY_RESOURCE_REQUEST,
  MEMORY_RESOURCE_RELEASE,
};


class MessageTypes {
 public:
  static Any createMemoryResourceRequest(size_t size) {
    MemoryResourceRequest resReq;
    resReq.set_size(size);
    resReq.set_type(MessageTypesEnum::MEMORY_RESOURCE_REQUEST);
    Any anyMessage;
    anyMessage.PackFrom(resReq);
    return anyMessage;
  }

  static Any createMemoryResourceRequest(const size_t size, const string& name,
                                         bool persistent) {
    MemoryResourceRequest resReq;
    resReq.set_size(size);
    resReq.set_type(MessageTypesEnum::MEMORY_RESOURCE_REQUEST);
    resReq.set_name(name);
    resReq.set_persistent(persistent);

    Any anyMessage;
    anyMessage.PackFrom(resReq);
    return anyMessage;
  }

  static Any createMemoryResourceRelease(size_t size, size_t offset) {
    MemoryResourceRequest resReq;
    resReq.set_size(size);
    resReq.set_offset(offset);
    resReq.set_type(MessageTypesEnum::MEMORY_RESOURCE_RELEASE);
    Any anyMessage;
    anyMessage.PackFrom(resReq);
    return anyMessage;
  }


  static Any createUpdateNID(
      const map<uint32_t, pair<string, uint32_t>>& nodeIDs) {

    UpdateNIDReq updateReq;
    for (auto& kv : nodeIDs) {
      updateReq.add_nodeid(kv.first);
      updateReq.add_connection(kv.second.first);
      updateReq.add_type(kv.second.second);
    }
    Any anyMessage;
    anyMessage.PackFrom(updateReq);
    return anyMessage;
  }

//  uint64 buffer = 1;
//    uint32 rkey = 2;
//    uint32 qp_num = 3;
//    uint32 lid = 4;
//    repeated uint32 gid = 5 [packed=true];
//    uint32 psn = 6;

  // static RDMAConnRequest getRDMAFromCN(ComputeNodeRDMAConnRequest& cnrr ){
  //     RDMAConnRequest ret;
  //     ret.CopyFrom((cnrr.rdmaconn()));
  //     return ret;
  // }

  // static ComputeNodeRDMAConnRequest setCNFromRDMA(RDMAConnRequest* conreq ){
  //     ComputeNodeRDMAConnRequest ret;
  //     ret.mutable_rdmaconn()->CopyFrom(*conreq);
  //     return ret;
  // }


};
// end class

}// end namespace dpi

#endif /* MESSAGETYPES_H_ */
