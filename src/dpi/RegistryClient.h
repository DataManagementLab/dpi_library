/**
 * @brief 
 * 
 * @file RegistryClient.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"

#include "../net/proto/ProtoClient.h"
#include "../../net/message/MessageTypes.h"
#include "../../net/message/MessageErrors.h"

#include "BufferHandle.h"

namespace dpi
{

class RegistryClient: public ProtoClient
{

  public:
    RegistryClient();
    virtual ~RegistryClient();

    virtual BufferHandle *createBuffer(string &name, NodeID node_id, size_t size, size_t threshold);
    virtual bool registerBuffer(BufferHandle *handle);
    virtual BufferHandle *retrieveBuffer(string &name);
    virtual bool appendSegment(string &name, BufferSegment &segment);

  private:
    bool appendOrRetrieveSegment(Any* sendAny);
    ProtoClient *m_client;
};

} // namespace dpi