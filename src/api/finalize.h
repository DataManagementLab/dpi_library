#pragma once

#include <string>
#include "context.h"
#include "err_codes.h"
#include "../utils/Config.h"

//Finalize for a specific buffer (name given) or all buffer (no name given)
inline int DPI_Finalize(CONTEXT& context, string const & name = "")
{
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;
    if (name != "")
    {
        auto buffer_writer = context.buffer_writers[name]; 
        buffer_writer->close();
        return DPI_SUCCESS;
    }
    for (auto &buffer_writer : context.buffer_writers)
    {
        buffer_writer.second->close();
    }
    return DPI_SUCCESS;
}