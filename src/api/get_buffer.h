#pragma once

#include <string>
#include "context.h"
#include "err_codes.h"
#include "../utils/Config.h"

int DPI_Get_buffer(string& name, CONTEXT& context)
{
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;
    
    auto buffer_writer = context.buffer_writers[name]; 
}