#pragma once

#include <string>
#include "context.h"
#include "err_codes.h"
#include "../utils/Config.h"

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