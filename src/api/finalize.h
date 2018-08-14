#pragma once

#include <string>
#include "context.h"
#include "err_codes.h"
#include "../utils/Config.h"

inline int DPI_Finalize(DPI_Context& context)
{
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;
    
    for (auto &buffer_writer : context.buffer_writers)
    {
        buffer_writer.second->close();
    }

    //todo: discuss how to properly free all allocated memory on all nodes?

    return DPI_SUCCESS;
}