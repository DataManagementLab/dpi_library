/** 
 * @file BufferHandle.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"

namespace dpi
{

struct BufferSegment
{
    size_t offset;
    size_t size; //Size of data portion (without header)
    size_t threshold;

    BufferSegment(){};
    BufferSegment(size_t offset, size_t size, size_t threshold) : offset(offset), size(size), threshold(threshold){};
};


struct BufferHandle
{
    string name;
    NodeID node_id;
    std::vector<BufferSegment> segments;

    BufferHandle(){};
    BufferHandle(string name, NodeID node_id) : name(name), node_id(node_id){};
};

} // namespace dpi