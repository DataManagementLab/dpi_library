#pragma once

#include "context.h"
#include "err_codes.h"

inline int DPI_Append(string& name, void* data, size_t size, CONTEXT& context)
{
    if (context.registry_client == nullptr) return DPI_NOT_INITIALIZED;
    
    auto buffer_writer = context.buffer_writers[name]; 
    // if (buffer_writer_pair == context.buffer_writers.end()) //No buffer_writer found
    // {
    //     auto buffer_handle = context.registry_client->retrieveBuffer(name);
    //     #ifdef DPI_STRATEGY_PRIVATE //todo: remove ugly hack
    //         BufferWriter<BufferWriterPrivate> buffer_writer = new BufferWriter<BufferWriterPrivate>(buffer_handle, Config::DPI_INTERNAL_BUFFER_SIZE, context.registry_client);
    //     #endif
    //     #ifdef DPI_STRATEGY_SHARED
    //         buffer_writer = new BufferWriter<BufferWriterShared>(buffer_handle, Config::DPI_INTERNAL_BUFFER_SIZE, context.registry_client);
    //     #endif
    //     context.buffer_writers[name] = buffer_writer;
    // }
    if (!buffer_writer->append(data, size))
        return DPI_FAILURE;
    return DPI_SUCCESS;
};