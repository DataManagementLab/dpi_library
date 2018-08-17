/**
 * @file init.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once
#include "context.h"
#include "err_codes.h"

/**
 * @brief DPI_Init initializes the DPI context
 * 
 * @param context - DPI_Context
 * @return int - error code
 */
inline int DPI_Init(DPI_Context& context)
{
    context.registry_client = new RegistryClient();
    return DPI_SUCCESS;
};