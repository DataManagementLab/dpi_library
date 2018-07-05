#pragma once

#include "../utils/Config.h"

#include "BuffHandle.h"
#include "../net/proto/ProtoClient.h"


namespace dpi
{
    
class RegistryClient
{

public:
    RegistryClient(){};
    ~RegistryClient(){};

    BuffHandle* dpi_create_buffer(string& name, NodeID node_id, size_t size){
        return nullptr;
    };    

    bool dpi_register_buffer(BuffHandle* handle){
        return true;
    };    
    BuffHandle* dpi_retrieve_buffer(string& name){
        return nullptr;
    };
    bool dpi_append_segment(string& name, BuffSegment& segment){
        return true;
    }; //TBD

private: 
    ProtoClient* m_client;
};

}