/**
 * @file finalize.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include <string>
#include "context.h"
#include "err_codes.h"
#include "../utils/Config.h"

/**
 * @brief DPI_Finalize indicates that client is finished, and free's up allocated memory
 * 
 * @param context - DPI_Context
 * @return int - error code
 */
inline int DPI_Finalize(DPI_Context& context)
{
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;
    
    for (auto &buffer_writer : context.buffer_writers)
    {
        buffer_writer.second->close();
    }
    return DPI_SUCCESS;
}