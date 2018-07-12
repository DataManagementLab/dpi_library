/**
 * @brief 
 * 
 * @file RegistryServer.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"

#include "../net/proto/ProtoServer.h"
#include "../net/rdma/RDMAClient.h"
#include "../message/MessageTypes.h"
#include "../message/MessageErrors.h"
#include "BufferHandle.h"

namespace dpi
{

class RegistryServer : public ProtoServer
{
public:
  // Constructors and Destructors
  RegistryServer();
  RegistryServer(RegistryServer &&) = default;
  RegistryServer(const RegistryServer &) = default;
  RegistryServer &operator=(RegistryServer &&) = default;
  RegistryServer &operator=(const RegistryServer &) = default;
  ~RegistryServer();

  /**
     * @brief handle messages 
     *  
     * @param sendMsg the incoming message
     * @param respMsg the response message
     */
  void handle(Any *sendMsg, Any *respMsg);

private:
  // Members
  RDMAClient *m_rdmaClient;
  map<string, BufferHandle> m_bufferHandles;

  // Methods

  /**
     * @brief Creates a new dpi buffer and allocates on segment on node with node_id
     * 
     * @param name of the buffer (unique)
     * @param node_id of the server where the buffer is allocated
     * @param size in bytes of the buffer
     */
  bool registerBuffer(BufferHandle *buffHandle);

  BufferHandle *createBuffer(string &name, NodeID node_id, size_t size, size_t threshold);

  BufferHandle *retrieveBuffer(string &name);

  bool appendSegment(string &name, BufferSegment *segment);
};

} // namespace dpi