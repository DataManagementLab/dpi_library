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

#include "BuffHandle.h"

namespace dpi
{
template <class base>
class BufferWriter : public base
{

  public:
    BufferWriter(BuffHandle *handle, size_t scratchPadSize = Config::DPI_SCRATCH_PAD_SIZE, RegistryClient *regClient = nullptr)
        : base(handle, scratchPadSize, regClient){};
    ~BufferWriter(){};

    //Append without use of scratchpad. Bad performance!
    bool append(void *data, size_t size)
    {
        while (size > this->m_scratchPadSize)
        {
            // update size move pointer
            size = size - this->m_scratchPadSize;
            memcpy(this->m_scratchPad, data, this->m_scratchPadSize);
            data = ((char *)data + this->m_scratchPadSize);
            if (!this->super_append(this->m_scratchPadSize))
                return false;
        }
        memcpy(this->m_scratchPad, data, size);
        return this->super_append(size);
    }

    //Append with scratchpad
    bool appendFromScratchpad(size_t size)
    {
        if (size > this->m_scratchPadSize)
        {
            std::cerr << "The size specified exceeds size of scratch pad size" << std::endl;
            return false;
        }
        return this->super_append(size);
    }

    void *getScratchPad()
    {
        return this->m_scratchPad;
    }
};

} // namespace dpi