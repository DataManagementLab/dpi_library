#include "RegistryServer.h"
    
    
    RegistryServer::RegistryServer(): ProtoServer("Registry Server", Config::DPI_REGISTRY_PORT) {};
    RegistryServer::RegistryServer(int port): ProtoServer("Registry Server",port){};
    RegistryServer::~RegistryServer(){};

    void RegistryServer::handle(Any* sendMsg, Any* respMsg){};


    bool RegistryServer::dpi_register_buffer(BuffHandle* handle){};

    BuffHandle* RegistryServer::dpi_create_buffer(string& name, NodeID node_id, size_t size){};
    
    BuffHandle* RegistryServer::dpi_retrieve_buffer(string& name){};

    bool RegistryServer::dpi_append_segment(string& name, BuffSegment* segment){};