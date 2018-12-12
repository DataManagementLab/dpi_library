/**
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

    // virtual BufferHandle *createBuffer(string &name, NodeID node_id, size_t size, size_t threshold);

    /**
     * @brief Registers the buffer at the RegistryServer --> creates a mapping from name to BufferHandle
     * 
     * @param handle - BufferHandle containing meta buffer data
     * @return true - if Buffer was registered
     * @return false - if Buffer was not registered
     */
    virtual bool registerBuffer(BufferHandle *handle);

    /**
     * @brief retrieves the BufferHandle identified by name. 
     * 
     * @param name of buffer
     * @return BufferHandle* - BufferHandle will contain a full list of all entry-segments into all rings
     */
    virtual BufferHandle *retrieveBuffer(string &name);
    
    /**
     * @brief Create a Segment Ring On Buffer specific for one BufferWriter
     * 
     * @param name of buffer
     * @return BufferHandle* - BufferHandle only contains one element in the vector with the entry segment to the ring
     */
    virtual BufferHandle *createSegmentRingOnBuffer(string &name);

    // virtual bool appendSegment(string &name, BufferSegment &segment);

  private:
    bool appendOrRetrieveSegment(Any* sendAny);
    ProtoClient *m_client;
};

} // namespace dpi