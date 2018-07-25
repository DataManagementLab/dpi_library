#pragma once
#include "context.h"

inline int DPI_Init(CONTEXT& context)
{
    context.registry_client = new RegistryClient();
};