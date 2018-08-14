#pragma once

#include <string>
#include "context.h"
#include "err_codes.h"
#include "../utils/Config.h"

inline int DPI_Close_buffer(string& name, DPI_Context& context)
{
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;
    
    auto buffer_writer = context.buffer_writers[name]; 
    buffer_writer->close();
    
    return DPI_SUCCESS;
}