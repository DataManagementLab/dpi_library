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

#include "BuffHandle.h"

namespace dpi
{

class RegistryClient: public ProtoClient
{

  public:
    RegistryClient();
    virtual ~RegistryClient();

    virtual BuffHandle *dpi_create_buffer(string &name, NodeID node_id, size_t size, size_t threshold);
    virtual bool dpi_register_buffer(BuffHandle *handle);
    virtual BuffHandle *dpi_retrieve_buffer(string &name);
    virtual bool dpi_append_segment(string &name, BuffSegment &segment);

    virtual BuffHandle* dpi_create_buffer(string& name, NodeID node_id, string& connection){};


  private:
    bool dpi_append_or_retrieve_segment(Any* sendAny);
    ProtoClient *m_client;
};

} // namespace dpi