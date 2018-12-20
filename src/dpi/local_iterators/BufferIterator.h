
#pragma once

#include "../../utils/Config.h"
#include "../BufferSegment.h"
#include <list>
namespace dpi
{
class BufferIterator
{

  public:
    // empty Iterator
    BufferIterator()
    {
        //TBI
    }

    BufferIterator(char *rdmaBufferPtr, std::vector<BufferSegment> &entrySegments, string &bufferName)
    {
        for (auto &eSeg : entrySegments)
        {
            segment_iterators.emplace_back(eSeg.begin(rdmaBufferPtr), eSeg.end(), eSeg.offset - sizeof(uint64_t)); // 3rd argument --> BufferIteratorLat specific, todo: split up properly!!!
        }
        pointer_to_iter = segment_iterators.begin();
        buffer_name = bufferName;
    };

    bool operator==(BufferIterator other) const { return (buffer_name == other.buffer_name); }
    bool operator!=(BufferIterator other) const { return !(*this == other); }

    //blocking check if next Element is there; will move the iterator to next consumable segment
    // returns false if empty
    bool has_next()
    {
        if (segment_iterators.empty())
        {
            return false;
        }

        while (!segment_iterators.empty())
        {
            if (pointer_to_iter == segment_iterators.end())
            {
                pointer_to_iter = segment_iterators.begin();
            }
            // check if end
            if (!(*pointer_to_iter).prev->isEndSegment())
            {
                if (!(*pointer_to_iter).iter->isConsumable())
                {
                    //advance iter to next ring
                    pointer_to_iter++;
                    // std::cout << "Segment in ring was not consumable, advancing" << '\n';
                }
                else
                {

                    return true;
                }
            }
            else
            {
                mark_prev_segment();
                // remove from segment iterators
                segment_iterators.erase(pointer_to_iter++);
            }
        }
        return false;
    }


    void *next(size_t &ret_size)
    {
        mark_prev_segment();

        //increment SegmentIterator
        void *ret_ptr = (*pointer_to_iter).iter.getRawData(ret_size);

        // save prev element for reuse
        (*pointer_to_iter).prev = (*pointer_to_iter).iter;

        (*pointer_to_iter).iter++;
        // proceed to next ring for next call
        pointer_to_iter++;
        return ret_ptr;
    }
    // blocking call combines has_next and next
    void *wait_for_next(size_t &ret_datasize, bool &ret_bufferOpen)
    {
        if (has_next())
        {
            ret_bufferOpen = true;
            return next(ret_datasize);
        }
        ret_bufferOpen = false;
        return nullptr;
    }

  protected:
    virtual void mark_prev_segment() //todo fix virtual
    {
        if (!(*pointer_to_iter).prev->isEndSegment())
        {
            (*pointer_to_iter).prev->counter = 0;
            (*pointer_to_iter).prev->setConsumable(false);
            (*pointer_to_iter).prev->setWriteable(true);
        }
    }

    struct IterHelper
    {
        SegmentIterator iter;
        SegmentIterator end;
        SegmentIterator prev;
        size_t counterOffset;
        IterHelper(SegmentIterator iter, SegmentIterator end, size_t counterOffset = 0) : iter(iter), end(end), prev(end), counterOffset(counterOffset)
        {
            prev->markEndSegment(false);
        };
    };

    std::list<IterHelper> segment_iterators;
    list<IterHelper>::iterator pointer_to_iter; // points to iterator with next data segment
    string buffer_name;
};


class BufferIteratorLat : public BufferIterator
{
  public:
    // BufferIteratorLat(){};
    BufferIteratorLat(char *rdmaBufferPtr, std::vector<BufferSegment> &entrySegments, string &bufferName) : BufferIterator(rdmaBufferPtr, entrySegments, bufferName)
    {
        this->rdmaBufferPtr = rdmaBufferPtr;
    };

  protected:

    void mark_prev_segment()
    {       
        if ((!(*pointer_to_iter).prev->isEndSegment()) && (*pointer_to_iter).prev->isWriteable())
        {
            (*pointer_to_iter).prev->counter = 0;
            (*pointer_to_iter).prev->setConsumable(false);
            (*pointer_to_iter).prev->setWriteable(true);
        
            //Increment the consumed segments counter
            ++(*(uint64_t*) ((*pointer_to_iter).counterOffset + rdmaBufferPtr));
            // std::cout << "New counter value: " << *(uint64_t*) ((*pointer_to_iter).counterOffset + rdmaBufferPtr) << " counter offset: " << (*pointer_to_iter).counterOffset << '\n';
        }        
    }

    char *rdmaBufferPtr = nullptr;

};


} // namespace dpi