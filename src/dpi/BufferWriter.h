/**
 * @brief 
 * 
 * @file BufferWriter.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */
#pragma once
#include "../utils/Config.h"
#include "../net/rdma/RDMAClient.h"
#include "RegistryClient.h"
#include "BuffHandle.h"

namespace dpi
{
template <class base>
class BufferWriter : public base 
{

public:
    BufferWriter(BuffHandle* handle): m_handle(handle) {};
    ~BufferWriter(){};

    bool append(void* data, size_t size){
        return this->super_append(m_handle, data, size);
    }
private:
    BuffHandle* m_handle;
    
};


class BufferWriterShared{


public:
    BufferWriterShared(){};
    ~BufferWriterShared(){};
    bool super_append(BuffHandle* handle, void* data, size_t size){
        return true;
    }

    private:    
    RegistryClient* m_reg_client;
    RDMAClient* m_rdma_client; // used to issue RDMA req. to the NodeServer
};


} // namespace 