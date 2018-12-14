/**
 * @file SegmentIterator.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-12-10
 */

#pragma once

#include "../../utils/Config.h"
#include <algorithm>

namespace dpi
{
class SegmentIterator : public std::iterator<
                            std::input_iterator_tag,              // iterator_category
                            Config::DPI_SEGMENT_HEADER_t,         // value_type
                            Config::DPI_SEGMENT_HEADER_t,         // difference_type
                            const Config::DPI_SEGMENT_HEADER_t *, // pointer
                            Config::DPI_SEGMENT_HEADER_t          // reference
                            >
{

    using header_ptr = Config::DPI_SEGMENT_HEADER_t *;

  public:
    // empty Iterator
    explicit SegmentIterator(){
        current_header = new Config::DPI_SEGMENT_HEADER_t();
        current_header->hasFollowSegment = 0;
        prevHasFollowSegment = 0;
    }

    explicit SegmentIterator(size_t offset, char *rdmaBufferPtr) : current_position(offset), m_rdmaBuffer(rdmaBufferPtr)
    {
        current_header = (header_ptr)(m_rdmaBuffer + offset);
        prevHasFollowSegment = 1;
    };
    SegmentIterator &operator++()
    {
        current_position = current_header->nextSegmentOffset;
        prevHasFollowSegment = current_header->hasFollowSegment;
        current_header = (header_ptr)(m_rdmaBuffer + current_position);
        
        return *this;
    }
    SegmentIterator operator++(int)
    {
        SegmentIterator retval = *this;
        ++(*this);
        return retval;
    }

    bool operator==(SegmentIterator other) const { return (prevHasFollowSegment == other.prevHasFollowSegment) && (current_position == other.current_position); }
    bool operator!=(SegmentIterator other) const {return !(*this == other);}
    reference operator*() const { return *current_header; }
    pointer operator->() const { return current_header;}

    void* getRawData(size_t& dataSizeInBytes){
        dataSizeInBytes = current_header->counter;
        return (void*) (current_header + 1); // get to raw data by ignoring header
    }


  private:
    size_t current_position = 0;
    size_t prevHasFollowSegment = 1;
    Config::DPI_SEGMENT_HEADER_t *current_header = nullptr;
    char *m_rdmaBuffer = nullptr;
};

} // namespace dpi