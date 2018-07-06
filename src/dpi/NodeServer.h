/**
 * @brief 
 * 
 * @file NodeServer.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"
#include "../net/rdma/RDMAServer.h"

namespace dpi
{

class NodeServer : public RDMAServer
{
private:
    /* data */
public:
    NodeServer(/* args */);
    NodeServer(uint16_t port);
    ~NodeServer();
};

}