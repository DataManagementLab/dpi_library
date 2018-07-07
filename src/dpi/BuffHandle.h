/**
 * @brief 
 * 
 * @file BuffHandle.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"

namespace dpi
{

struct BuffSegment
{
    size_t offset;
    size_t size;
    size_t threshold;

    BuffSegment(){};
    
    BuffSegment(size_t offset,size_t size,size_t threshold) : 
    offset(offset), size(size), threshold(threshold){};
};
struct BuffHandle
{
    string name;
    NodeID node_id;
    string& connection;
    std::vector<BuffSegment> segments;

    BuffHandle(string name, NodeID node_id, string connection):name(name), node_id(node_id), connection(connection){};
};

}