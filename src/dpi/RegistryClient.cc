#include "RegistryClient.h"


 RegistryClient::RegistryClient(){};
    RegistryClient::~RegistryClient(){};

    BuffHandle* RegistryClient::dpi_create_buffer(string& name, NodeID node_id, string& connection){
        return nullptr;
    };    

    bool RegistryClient::dpi_register_buffer(BuffHandle* handle){
        return true;
    };    
    BuffHandle* RegistryClient::dpi_retrieve_buffer(string& name){
        return nullptr;
    };
    bool RegistryClient::dpi_append_segment(string& name, BuffSegment& segment){
        return true;
    }; //TBD