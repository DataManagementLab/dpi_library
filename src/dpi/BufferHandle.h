/** 
 * @file BufferHandle.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"
#include "local_iterators/SegmentIterator.h"
#include "local_iterators/BufferIterator.h"
#include "BufferSegment.h"

namespace dpi
{

struct BufferHandle
{
    string name;
    NodeID node_id;
    std::vector<BufferSegment> entrySegments; //Each entry in the vector corresponds to one ring for a writer
    size_t segmentsPerWriter; //Number of segments in ring that will be created for each writer when createSegmentRingOnBuffer() is called
    size_t segmentSizes; //in bytes (excluding header)
    size_t numberOfAppenders;

    BufferHandle(){};
    BufferHandle(string name, NodeID node_id, size_t segmentsPerWriter,size_t numberOfAppenders,size_t segmentSizes = Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) 
        : name(name), node_id(node_id), segmentsPerWriter(segmentsPerWriter), numberOfAppenders(numberOfAppenders) ,segmentSizes(segmentSizes){};


    BufferIterator getIterator(char* rdmaBuffer){
        return BufferIterator(rdmaBuffer, entrySegments, name);
    }
};

} // namespace dpi