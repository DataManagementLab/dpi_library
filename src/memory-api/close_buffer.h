/**
 * @file close_buffer.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include <string>
#include "context.h"
#include "err_codes.h"
#include "../utils/Config.h"

/**
 * @brief DPI_Close_buffer indicates that the calling client does not wish to send more data to the buffer. Buffer is not remotely removed and can still be read from.
 * 
 * @param name - name of the buffer
 * @param context - DPI_Context
 * @return int - error code
 */
inline int DPI_Close_buffer(string& name, DPI_Context& context)
{
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;
    
    auto buffer_writer = context.buffer_writers[name]; 
    buffer_writer->close();
    
    return DPI_SUCCESS;
}