/**
 * @file NodeServer.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"
#include "../net/rdma/RDMAServer.h"
#include <mutex> // For std::unique_lock
#include <shared_mutex>

namespace dpi
{

class NodeServer : public RDMAServer
{
  private:
  
  public:
    NodeServer(/* args */);
    NodeServer(uint16_t port);
    ~NodeServer();

    void handle(Any *sendMsg, Any *respMsg)
    {
        if (sendMsg->Is<DPIAllocSegmentsRequest>())
        {
            Logging::debug(__FILE__, __LINE__, "Got DPIAllocSegmentsRequest msg");
            DPIAllocSegmentsRequest reqMsgUnpacked;
            DPIAllocSegmentsResponse respMsgUnpacked;

            sendMsg->UnpackTo(&reqMsgUnpacked);
            size_t segmentsCount = reqMsgUnpacked.segments_count();
            size_t segmentsSize = reqMsgUnpacked.segments_size();
            string name = reqMsgUnpacked.name();
            size_t offset = 0;
            BufferHandle::Buffertype bufferType = static_cast<BufferHandle::Buffertype>(reqMsgUnpacked.buffer_type());

            Logging::debug(__FILE__, __LINE__, "Requesting Memory Resource, size: " + to_string(segmentsCount * segmentsSize));
            size_t allocateMemorySize = segmentsCount * segmentsSize;
            if (bufferType == BufferHandle::Buffertype::LAT)
            {
                allocateMemorySize += sizeof(uint64_t); //Add the counter
            }
            
            respMsgUnpacked.set_return_(requestMemoryResource(allocateMemorySize, offset));
            
            if (bufferType == BufferHandle::Buffertype::LAT)
            {
                uint64_t counter = segmentsCount;
                memcpy((char *)this->getBuffer() + offset, &counter, sizeof(uint64_t));
                // std::cout << "Wrote counter on offset: " << offset << " initial value: " << counter << '\n';
                offset += sizeof(int64_t); //shift the offset past the counter
            }
            respMsgUnpacked.set_offset(offset);

            for (size_t i = 0; i < segmentsCount; i++)
            {
                bool lastSegment = i == segmentsCount - 1;
                size_t nextSegOffset;
                if (lastSegment)
                    nextSegOffset = offset;
                else
                    nextSegOffset = offset + (i + 1) * segmentsSize;

                Config::DPI_SEGMENT_HEADER_t segmentHeader;
                segmentHeader.counter = 0;
                segmentHeader.nextSegmentOffset = nextSegOffset;
                segmentHeader.setWriteable(true);
                //Write the header
                memcpy((char *)this->getBuffer() + offset + i * segmentsSize, &segmentHeader, sizeof(segmentHeader));
            }
            respMsg->PackFrom(respMsgUnpacked);

            Logging::debug(__FILE__, __LINE__, "Finished DPIAllocSegmentsRequest");
        }
        else
        {
            RDMAServer::handle(sendMsg, respMsg);
        }
    }
};

} // namespace dpi