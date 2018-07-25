#pragma once

#include "../utils/Config.h"
#include <unordered_map>
#include "../dpi/BufferWriter.h"
#include "../dpi/RegistryClient.h"

struct CONTEXT
{
    #ifdef DPI_STRATEGY_PRIVATE //todo: remove ugly hack
    std::unordered_map<std::string, BufferWriter<BufferWriterPrivate>*> buffer_writers;
    #endif
    #ifdef DPI_STRATEGY_SHARED
    std::unordered_map<std::string, BufferWriter<BufferWriterShared>*> buffer_writers;
    #endif

    RegistryClient* registry_client = nullptr;
};