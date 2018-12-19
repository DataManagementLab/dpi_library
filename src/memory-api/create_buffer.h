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
 * @param segmentsPerAppender - The number of segments each appender get allocated in the buffer
 * @param participatingAppenders - The number of appenders that are going to append to the buffer
 * @return int - error code
 */
inline int DPI_Create_buffer(std::string &name, NodeID node_id, DPI_Context& context, size_t segmentsPerAppender = 1, size_t participatingAppenders = 1)
{
    //Check that DPI_Init has been called
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;

    context.registry_client->registerBuffer(new BufferHandle(name, node_id, segmentsPerAppender, participatingAppenders, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)));
    BufferHandle *buffHandle = context.registry_client->retrieveBuffer(name);
    context.buffer_writers[name] = new BufferWriterBW(name, context.registry_client, Config::DPI_INTERNAL_BUFFER_SIZE);
    
    context.buffer_readers[name] = new BufferReader(buffHandle, context.registry_client);
    return DPI_SUCCESS;
    
};