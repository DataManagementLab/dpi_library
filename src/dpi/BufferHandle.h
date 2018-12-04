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
    bool reuseSegments; //true: A consumer needs to be present to iterate over the segment-ring and segments will be reused, false: segments will not be reused and can grow indefinitely (larger than segmentsPerWriter)
    size_t segmentSizes; //in bytes (excluding header)

    BufferHandle(){};
    BufferHandle(string name, NodeID node_id, size_t segmentsPerWriter, bool reuseSegments = false, size_t segmentSizes = Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) 
        : name(name), node_id(node_id), segmentsPerWriter(segmentsPerWriter), reuseSegments(reuseSegments), segmentSizes(segmentSizes){};
};

} // namespace dpi