/**
 * @brief 
 * 
 * @file RegistryClient.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */


#pragma once

#include "../utils/Config.h"

#include "BuffHandle.h"
#include "../net/proto/ProtoClient.h"


namespace dpi
{
    
class RegistryClient
{

public:
    RegistryClient();
    ~RegistryClient();

    BuffHandle* dpi_create_buffer(string& name, NodeID node_id, size_t size);    

    bool dpi_register_buffer(BuffHandle* handle);    
    BuffHandle* dpi_retrieve_buffer(string& name);
    bool dpi_append_segment(string& name, BuffSegment& segment); //TBD

private: 
    ProtoClient* m_client;
};

}