/** 
 * @file BufferSegment.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"
#include "local_iterators/SegmentIterator.h"

namespace dpi
{



/**
 * @brief BufferSegments are stored in a ring-buffer fashion, with the Config::DPI_SEGMENT_HEADER_t holding a ptr to the next segment in the ring
 * 
 */
struct BufferSegment //todo: clean up in redundant data between SegmentHeader and BufferSegment types
{
    size_t offset;
    size_t size; //Size of data portion (without header)

    BufferSegment(){};
    BufferSegment(size_t offset, size_t size) : offset(offset), size(size){};


    SegmentIterator begin(char *rdmaBuffer)
    {
        return SegmentIterator(offset, rdmaBuffer);
    }

    SegmentIterator end()
    {
        return SegmentIterator::getEndSegmentIterator();
    }
};

}