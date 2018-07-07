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
    virtual ~RegistryClient();

    virtual BuffHandle* dpi_create_buffer(string& name, NodeID node_id, string& connection);

    virtual bool dpi_register_buffer(BuffHandle* handle);    
    virtual BuffHandle* dpi_retrieve_buffer(string& name);
    virtual bool dpi_append_segment(string& name, BuffSegment& segment); //TBD

private: 
    ProtoClient* m_client;
};

}