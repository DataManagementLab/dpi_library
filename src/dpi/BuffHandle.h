#pragma once

#include "../utils/Config.h"

namespace dpi
{

struct BuffSegment
{
    size_t offset;
    size_t size;
    size_t threshold;
};

struct BuffHandle
{
    string name;
    NodeID node_id;
    std::vector<BuffSegment> segments;

    BuffHandle(string name, NodeID node_id):name(name), node_id(node_id){};
};

}