/**
 * @brief BufferWriter implements two different RDMA strategies to write into the remote memory
 * 
 * @file BufferWriter.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"
#include "../utils/Network.h"
#include "../net/rdma/RDMAClient.h"
#include "RegistryClient.h"
#include "buffer_strategies/BufferStrategies.h"

#include "BufferHandle.h"

namespace dpi
{




template <class base>
class BufferWriter : public base
{

  public:
    BufferWriter(BufferHandle *handle, size_t internalBufferSize = Config::DPI_INTERNAL_BUFFER_SIZE, RegistryClient *regClient = nullptr)
        : base(handle, internalBufferSize, regClient){};
    ~BufferWriter(){};

    //data: ptr to data, size: size in bytes. return: true if successful, false otherwise
    bool append(void *data, size_t size)
    {
        while (size > this->m_internalBuffer->size)
        {
            // update size move pointer
            size = size - this->m_internalBuffer->size;
            if (!this->super_append(data, this->m_internalBuffer->size))
                return false;
            data = ((char *)data + this->m_internalBuffer->size);
        }
        return this->super_append(data, size);
    }

    bool addToInternalBuffer(void *data, size_t size)
    {
        return this->super_addToInternalBuffer(data, size);
    }

    bool close(){
        return this->super_close();
    }
};

} // namespace dpi