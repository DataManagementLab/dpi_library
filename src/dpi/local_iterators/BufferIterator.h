
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
            segment_iterators.emplace_back(eSeg.begin(rdmaBufferPtr), eSeg.end());
        }
        pointer_to_iter = segment_iterators.begin();
        buffer_name = bufferName;
    };

    bool operator==(BufferIterator other) const { return (buffer_name == other.buffer_name); }
    bool operator!=(BufferIterator other) const { return !(*this == other); }

    //blocking check if next Element is there;
    // returns falls if empty
    bool has_next()
    {

        if (segment_iterators.empty())
        {
            return false;
        }

        while (pointer_to_iter != segment_iterators.end())
        {
            // check if end
            if ((*pointer_to_iter).iter != (*pointer_to_iter).end)
            {
                if (!(*(*pointer_to_iter).iter).isConsumable())
                {
                    // std::cout << "Writer was not consumable!" << '\n';
                    continue;
                }

                return true;
            }
            else
            {
                mark_prev_segment();
                // remove from segment iterators
                segment_iterators.erase(pointer_to_iter++);
                continue;
            }
        }
        pointer_to_iter = segment_iterators.begin();
        return has_next();
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

  private:
    void mark_prev_segment()
    {
        if ((*pointer_to_iter).prev != (*pointer_to_iter).end)
        {
            std::cout << "Before updating prev iter: counter: " << (*pointer_to_iter).prev->counter << ", nextSegOffset: " << (*pointer_to_iter).prev->nextSegmentOffset << " flags: " << (*pointer_to_iter).prev->segmentFlags << '\n';
            (*pointer_to_iter).prev->counter = 0;
            (*pointer_to_iter).prev->setConsumable(false);
            (*pointer_to_iter).prev->setWriteable(true);
            std::cout << "After updating prev iter: counter: " << (*pointer_to_iter).prev->counter << ", nextSegOffset: " << (*pointer_to_iter).prev->nextSegmentOffset << " flags: " << (*pointer_to_iter).prev->segmentFlags << '\n';
        }
        else
        {
            std::cout << "Tried to update prev iter, but it was end!" << '\n';
        }
    }

    struct IterHelper
    {
        SegmentIterator iter;
        SegmentIterator end;
        SegmentIterator prev;
        IterHelper(SegmentIterator iter, SegmentIterator end) : iter(iter), end(end), prev(end){};
    };

    std::list<IterHelper> segment_iterators;
    list<IterHelper>::iterator pointer_to_iter; // points to iterator with next data segment
    string buffer_name;
};

} // namespace dpi