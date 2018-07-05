#pragma once

#include "../utils/Config.h"

#include "../net/proto/ProtoServer.h"
#include "../net/rdma/RDMAClient.h"


namespace dpi{

class RegistryServer : public ProtoServer
{
public:
    // Constructor and Destructor
    RegistryServer(){};
    RegistryServer(int port){};
    RegistryServer(RegistryServer &&) = default;
    RegistryServer(const RegistryServer &) = default;
    RegistryServer &operator=(RegistryServer &&) = default;
    RegistryServer &operator=(const RegistryServer &) = default;
    ~RegistryServer(){};


    /**
     * @brief handle messages 
     *  
     * @param sendMsg the incoming message
     * @param respMsg the response message
     */
    void handle(Any* sendMsg, Any* respMsg);

private:
    // Members
    RDMAClient* m_rdma_client;

    // Methods
    
    /**
     * @brief Allocates a new dpi buffer on node with node_id
     * 
     * @param name of the buffer (unique)
     * @param node_id of the server where the buffer is allocated
     * @param size in bytes of the buffer
     */
    bool dpi_register_buffer(BuffHandle* handle);

    BuffHandle* dpi_create_buffer(string& name, NodeID node_id, size_t size);
    
    BuffHandle* dpi_retrieve_buffer(string& name);

    bool dpi_append_segment(string& name, BuffSegment* segment);
};

} // end of namespace