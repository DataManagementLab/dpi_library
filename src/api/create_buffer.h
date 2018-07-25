#pragma once

#include <string>
#include "context.h"
#include "err_codes.h"
#include "../utils/Config.h"

inline int DPI_Create_buffer(std::string name, NodeID node_id, CONTEXT& context)
{
    //Check that everything is initialized, i.e. DPI_Init has been called
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;

    #ifdef DPI_STRATEGY_PRIVATE
        BufferHandle *buffHandle = new BufferHandle(name, node_id);
        context.registry_client->registerBuffer(buffHandle);
        context.buffer_writers[name] = new BufferWriter<BufferWriterPrivate>(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, context.registry_client);
        return DPI_SUCCESS;
    #endif
    #ifdef DPI_STRATEGY_SHARED
        BufferHandle *buffHandle = context.registry_client->createBuffer(name, node_id, (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)), Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR);
        context.buffer_writers[name] = new BufferWriter<BufferWriterShared>(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, context.registry_client);
        return DPI_SUCCESS;    
    #endif
    
    
};