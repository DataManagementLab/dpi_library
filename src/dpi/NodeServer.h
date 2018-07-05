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
    NodeServer(/* args */){};
    ~NodeServer(){};
};

}