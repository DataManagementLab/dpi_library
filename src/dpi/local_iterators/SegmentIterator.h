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
                            Config::DPI_SEGMENT_HEADER_t *, // pointer
                            Config::DPI_SEGMENT_HEADER_t&         // reference
                            >
{

    using header_ptr = Config::DPI_SEGMENT_HEADER_t *;

  public:
    // empty Iterator
    static SegmentIterator getEndSegmentIterator(){
        SegmentIterator endIter(0, nullptr);
        
        endIter.current_header = new Config::DPI_SEGMENT_HEADER_t();
        endIter.current_header->markEndSegment();
        endIter.prevIsEndSegment = true;
        endIter.current_position = 0;
        return endIter;
    }

    explicit SegmentIterator(size_t offset, char *rdmaBufferPtr) : current_position(offset), m_rdmaBuffer(rdmaBufferPtr)
    {
        current_header = (header_ptr)(m_rdmaBuffer + offset);
        prevIsEndSegment = false;
    };

    SegmentIterator& operator=(SegmentIterator other){
        current_position =  other.current_position;
        prevIsEndSegment =  other.prevIsEndSegment;
        current_header =  other.current_header;
        m_rdmaBuffer =  other.m_rdmaBuffer;
        return *this;
    }



    SegmentIterator &operator++()
    {
        current_position = current_header->nextSegmentOffset;
        prevIsEndSegment = current_header->isEndSegment();
        current_header = (header_ptr)(m_rdmaBuffer + current_position);
        return *this;
    }
    
    SegmentIterator operator++(int)
    {
        SegmentIterator retval = *this;
        ++(*this);
        return retval;
    }

    bool operator==(SegmentIterator other) const { return (prevIsEndSegment == other.prevIsEndSegment); }
    bool operator!=(SegmentIterator other) const {return !(*this == other);}
    reference operator*() { return *current_header; }
    pointer operator->() { return current_header;}

    

    void* getRawData(size_t& dataSizeInBytes){
        dataSizeInBytes = current_header->counter;
        return (void*) (current_header + 1); // get to raw data by ignoring header
    }


  private:
    size_t current_position = 0;
    bool prevIsEndSegment = false;
    Config::DPI_SEGMENT_HEADER_t *current_header = nullptr;
    char *m_rdmaBuffer = nullptr;
};

} // namespace dpi