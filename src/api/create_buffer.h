/**
 * @file create_buffer.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include <string>
#include "context.h"
#include "err_codes.h"
#include "../utils/Config.h"

/**
 * @brief DPI_Create_buffer initializes a buffer and needs to be called before any buffer operations
 * 
 * @param name - name of the buffer
 * @param node_id - NodeID of the node where the buffer should be located
 * @param context - DPI_Context
 * @return int - error code
 */
inline int DPI_Create_buffer(std::string &name, NodeID node_id, DPI_Context& context)
{
    //Check that DPI_Init has been called
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;

#ifdef DPI_STRATEGY_PRIVATE
    BufferHandle *buffHandle = new BufferHandle(name, node_id, 1);
    context.registry_client->registerBuffer(buffHandle);
    context.buffer_writers[name] = new BufferWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, context.registry_client);
#endif
#ifdef DPI_STRATEGY_SHARED
    BufferHandle *buffHandle = context.registry_client->createBuffer(name, node_id, (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)), Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR);
    context.buffer_writers[name] = new BufferWriter<BufferWriterShared>(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, context.registry_client);
#endif
    
    context.buffer_readers[name] = new BufferReader(buffHandle, context.registry_client);
    return DPI_SUCCESS;
    
};