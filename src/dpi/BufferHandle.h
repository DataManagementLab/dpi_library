/** 
 * @file BufferHandle.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"

namespace dpi
{


/**
 * @brief BufferSegments are stored in a ring-buffer fashion, with the DPI_SEGMENT_HEADER_t holding a ptr to the next segment in the ring
 * 
 */
struct BufferSegment //todo: clean up in redundant data between SegmentHeader and BufferSegment types
{
    size_t offset;
    size_t size; //Size of data portion (without header)
    size_t nextSegmentOffset;

    BufferSegment(){};
    BufferSegment(size_t offset, size_t size, size_t nextSegmentOffset = 0) : offset(offset), size(size), nextSegmentOffset(nextSegmentOffset){};
};


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
};

} // namespace dpi