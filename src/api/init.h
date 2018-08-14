#pragma once
#include "context.h"
#include "err_codes.h"

inline int DPI_Init(DPI_Context& context)
{
    context.registry_client = new RegistryClient();
    return DPI_SUCCESS;
};