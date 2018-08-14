#pragma once

#include "context.h"
#include "err_codes.h"

inline int DPI_Append(string& name, void* data, size_t size, DPI_Context& context)
{
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;
    
    auto buffer_writer = context.buffer_writers[name]; 

    if (!buffer_writer->append(data, size))
        return DPI_FAILURE;
    return DPI_SUCCESS;
};