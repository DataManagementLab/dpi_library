/**
 * @file append.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include "context.h"
#include "err_codes.h"
/**
 * @brief DPI_Append adds the data to the remotely located buffer
 * 
 * @param name - name of the buffer
 * @param data - void pointer to data location
 * @param size - size of the data, in bytes
 * @param context - DPI_Context
 * @return int - error code
 */
inline int DPI_Append(string& name, void* data, size_t size, DPI_Context& context)
{
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;
    
    auto buffer_writer = context.buffer_writers[name]; 

    if (!buffer_writer->append(data, size))
        return DPI_FAILURE;
    return DPI_SUCCESS;
};