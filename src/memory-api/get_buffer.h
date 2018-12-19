/**
 * @file get_buffer.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include <string>
#include "context.h"
#include "err_codes.h"
#include "../utils/Config.h"

/**
 * @brief DPI_Get_buffer pulls latest buffer state from RegistryServer and reads the entire buffer
 * 
 * @param name - name of the buffer
 * @param data_read - reference to an unsigned int, which will contain the size (in bytes) of the read buffer after return
 * @param data - reference to void pointer which will point to the data after return
 * @param context - DPI_Context
 * @return int - error code
 */
inline int DPI_Get_buffer(string& name, size_t &data_read, void *& data, DPI_Context& context)
{
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;
    
    auto a = context.buffer_readers.find(name);
    if (a == context.buffer_readers.end())
        std::cout << "Buffer reader not found in map" << '\n';

    auto buffer_reader = context.buffer_readers[name]; 
    
    data = buffer_reader->read(data_read);

    if (data != nullptr)
        return DPI_SUCCESS;
    return DPI_FAILURE;
}