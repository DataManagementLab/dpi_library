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
    ~RegistryClient();

    BuffHandle *dpi_create_buffer(string &name, NodeID node_id, size_t size, size_t threshold);
    bool dpi_register_buffer(BuffHandle *handle);
    BuffHandle *dpi_retrieve_buffer(string &name);
    bool dpi_append_segment(string &name, BuffSegment &segment);

  private:
    bool dpi_append_or_retrieve_segment(Any* sendAny);
    ProtoClient *m_client;
};

} // namespace dpi