/**
 * @brief BufferWriter implements two different RDMA strategies to write into the remote memory
 * 
 * @file BufferWriter.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */

#pragma once

#include "../utils/Config.h"
#include "../utils/Network.h"
#include "../net/rdma/RDMAClient.h"
#include "RegistryClient.h"
#include "BuffHandle.h"

namespace dpi
{
template <class base>
class BufferWriter : public base
{

  public:
    BufferWriter(BuffHandle *handle, size_t scratchPadSize = Config::DPI_SCRATCH_PAD_SIZE, RegistryClient *regClient = nullptr)
        : base(handle, scratchPadSize, regClient){};
    ~BufferWriter(){};

    //Append without use of scratchpad. Bad performance!
    bool append(void *data, size_t size)
    {
        while (size > this->m_scratchPadSize)
        {
            // update size move pointer
            size = size - this->m_scratchPadSize;
            memcpy(this->m_scratchPad, data, this->m_scratchPadSize);
            data = ((char *)data + this->m_scratchPadSize);
            if (!this->super_append(this->m_scratchPadSize))
                return false;
        }

        memcpy(this->m_scratchPad, data, size);
        return this->super_append(size);
    }

    //Append with scratchpad
    bool appendFromScratchpad(size_t size)
    {
        if (size > this->m_scratchPadSize)
        {
            std::cerr << "The size specified exceeds size of scratch pad size" << std::endl;
            return false;
        }

        return this->super_append(size);
    }

    void *getScratchPad()
    {
        return this->super_getScratchPad();
    }
};

class BufferWriterShared
{

  public:
    BufferWriterShared(BuffHandle *handle, size_t scratchPadSize, RegistryClient *regClient = nullptr) : m_handle(handle), m_scratchPadSize(scratchPadSize), m_regClient(regClient)
    {
        m_rdmaClient = new RDMAClient();
        m_rdmaClient->connect(handle->connection, handle->node_id);

        TO_BE_IMPLEMENTED(if (regClient == nullptr) {
            m_regClient = new RegistryClient();
        } m_regClient->connect(Config::DPI_REGISTRY_SERVER));
        m_scratchPad = m_rdmaClient->localAlloc(m_scratchPadSize);
        m_rdmaHeader = (Config::DPI_SEGMENT_HEADER_t *)m_rdmaClient->localAlloc(sizeof(Config::DPI_SEGMENT_HEADER_t));
    };

    ~BufferWriterShared()
    {
        if (m_scratchPad != nullptr)
        {
            m_rdmaClient->localFree(m_scratchPad);
        }
        m_scratchPad = nullptr;
        delete m_rdmaClient;
        TO_BE_IMPLEMENTED(delete m_regClient;)
    };

    bool super_append(size_t size)
    {
        std::cout << "Appending Shared Strategy" << '\n';
        

        if (m_handle->segments.empty())
        {
                std::cout << "Empty Segment" << '\n';
                size_t remoteOffset = 0;
                if (!m_rdmaClient->remoteAlloc(m_handle->connection, Config::DPI_SEGMENT_SIZE, remoteOffset))
                {
                    return false;
                }
                std::cout << "remoteOffset" << remoteOffset << '\n';
                BuffSegment newSegment(remoteOffset, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t), Config::DPI_SEGMENT_SIZE * 0.8);
                m_handle->segments.push_back(newSegment);
                m_regClient->dpi_append_segment(m_handle->name, newSegment);
        }
        auto segment = m_handle->segments.back();
        std::cout << "Segment " << segment.offset << '\n';
        auto fetchedCounterOld = modifyCounter(size, segment.offset);

        // data fits below threshold
        if (fetchedCounterOld + size < segment.threshold)
        {
            std::cout << "Case 1" << '\n';
            if (!m_rdmaClient->write(m_handle->node_id, segment.offset + fetchedCounterOld + sizeof(Config::DPI_SEGMENT_HEADER_t), m_scratchPad, size, true))
            {
                return false;
            }
            // data fits below in segment but is already above threshold -> someone else allocates memory
        }
        else if (fetchedCounterOld > segment.threshold && fetchedCounterOld + size <= segment.size)
        {
            std::cout << "Case 2" << '\n';
            if (!m_rdmaClient->write(m_handle->node_id, segment.offset + fetchedCounterOld + sizeof(Config::DPI_SEGMENT_HEADER_t), m_scratchPad, size, true))
            {
                return false;
            }
            // Fit into segment but passes threshold
            // else if( seg_size >= fetchedCounter + size > threshold && fetchedCounter <= threshhold) {
        }
        else if (segment.size >= fetchedCounterOld + size && fetchedCounterOld + size >= segment.threshold && fetchedCounterOld <= segment.threshold)
        {
            std::cout << "Case 3" << '\n';
            auto hasFollowPage = setHasFollowPage(segment.offset + sizeof(Config::DPI_SEGMENT_HEADER_t::counter));
            if (hasFollowPage == 0)
            {
                size_t remoteOffset = 0;
                if (!m_rdmaClient->remoteAlloc(m_handle->connection, Config::DPI_SEGMENT_SIZE, remoteOffset))
                {
                    return false;
                }
                std::cout << "remoteOffset" << remoteOffset << '\n';
                BuffSegment newSegment(remoteOffset, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t), Config::DPI_SEGMENT_SIZE * 0.8);
                m_regClient->dpi_append_segment(m_handle->name, newSegment);
            }
            if (!m_rdmaClient->write(m_handle->node_id, segment.offset + fetchedCounterOld + sizeof(Config::DPI_SEGMENT_HEADER_t), m_scratchPad, size, true))
            {
                return false;
            }
            // else if( fetchedCounter + size > seg_size){
        }
        else if (fetchedCounterOld + size > segment.size)
        {
            std::cout << "Case 4" << '\n';
            auto resetCounter = modifyCounter(-size, segment.offset);
            auto hasFollowPage = setHasFollowPage(segment.offset + sizeof(Config::DPI_SEGMENT_HEADER_t::counter));
            if (hasFollowPage == 0)
            {
                std::cout << "Case 4.1" << '\n';
                size_t remoteOffset = 0;
                if (!m_rdmaClient->remoteAlloc(m_handle->connection, Config::DPI_SEGMENT_SIZE, remoteOffset))
                {
                    return false;
                }
                BuffSegment newSegment(remoteOffset, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t), Config::DPI_SEGMENT_SIZE * 0.8);
                m_handle->segments.push_back(newSegment);
                m_regClient->dpi_append_segment(m_handle->name, newSegment);
                return super_append(size);
            }
            else
            {   
                
                std::cout << "Case 4.2" << '\n';
                m_handle = m_regClient->dpi_retrieve_buffer(m_handle->name);
                std::cout << "segment size of segments" << m_handle->segments.size() << '\n';
                std::cout << "segment 1 offset" << m_handle->segments[0].offset << '\n';
                std::cout << "segment 2 offset" << m_handle->segments[1].offset << '\n';
                return super_append(size);
            }
        }
        else
        {
            std::cout << "Case 5" << '\n';
            std::cout << "Special Case not handeled" << ((int *)m_scratchPad)[0] << '\n';
            return false;
        }

        /*
            size: size to write
            seg_size: size of segment
            //OLD Counter
            auto fetchedCounter = fetch & add counter

            if(fetchedCounter + size < threshhold){
                APPEND
            }
            else if(fetchedCounter > threshshold && fetchedCounter + size <= seg_size) {
                APPEND
            }

            //PASS THRESHOLD BUT FIT INTO SEGMENT

            else if( seg_size >= fetchedCounter + size > threshold && fetchedCounter <= threshhold) {
                auto hasFollowPage = C&S(setFollow)
                if(hashFollowPage == 0){
                    allocate new Segment
                    append_segment
                }
                APPEND OLD PAGE
            }
            else if( fetchedCounter + size > seg_size){
                fetch & add -counter
                auto hasFollowPage = C&S(setFollow)
                if(hashFollowPage == 0){
                    allocate new Segment  
                    start over  
                }else{
                    dpi_retrieve_buffer
                    start over?
                }
            }
        */
        return true;
    }

    void *super_getScratchPad()
    {
        return m_scratchPad;
    }

    inline int64_t modifyCounter(int64_t value, size_t offset)
    {

        while (!m_rdmaClient->fetchAndAdd(m_handle->node_id, offset, (void *)&m_rdmaHeader->counter,
                                          value, sizeof(uint64_t), true))
            ;

        return Network::bigEndianToHost(m_rdmaHeader->counter);
    }

    inline int64_t setHasFollowPage(size_t offset)
    {
        while (!m_rdmaClient->compareAndSwap(m_handle->node_id, offset, (void *)&m_rdmaHeader->hasFollowPage,
                                             0, 1, sizeof(uint64_t), true))
            ;

        return Network::bigEndianToHost(m_rdmaHeader->hasFollowPage);
    }

  protected:
    BuffHandle *m_handle = nullptr;
    void *m_scratchPad = nullptr;
    size_t m_scratchPadSize = 0;

    // helper Methods to atomic update
    // fetch add to counter

    // c&s to hasFollowPage

  private:
    // Scratch Pad
    RegistryClient *m_regClient = nullptr;
    Config::DPI_SEGMENT_HEADER_t *m_rdmaHeader = nullptr;
    RDMAClient *m_rdmaClient = nullptr; // used to issue RDMA req. to the NodeServer
};

class BufferWriterPrivate
{

  public:
    BufferWriterPrivate(BuffHandle *handle, size_t scratchPadSize, RegistryClient *regClient = nullptr) : m_handle(handle), m_scratchPadSize(scratchPadSize), m_regClient(regClient)
    {
        m_rdmaClient = new RDMAClient();
        m_rdmaClient->connect(handle->connection, handle->node_id);
        TO_BE_IMPLEMENTED(if (m_regClient == nullptr) {
            m_regClient = new RegistryClient();
        } m_regClient->connect(Config::DPI_REGISTRY_SERVER));
        m_scratchPad = m_rdmaClient->localAlloc(m_scratchPadSize);
    };

    ~BufferWriterPrivate()
    {
        if (m_scratchPad != nullptr)
        {
            m_rdmaClient->localFree(m_scratchPad);
        }
        m_scratchPad = nullptr;
        delete m_rdmaClient;
        TO_BE_IMPLEMENTED(delete m_regClient;)
    };

    // ToDo flag to change signaled.
    bool super_append(size_t size)
    {

        if (m_localBufferSegments.empty() || m_localBufferSegments.back().size - m_sizeUsed < size)
        {
            size_t remoteOffset = 0;
            if (!m_rdmaClient->remoteAlloc(m_handle->connection, Config::DPI_SEGMENT_SIZE, remoteOffset))
            {
                return false;
            }
            m_sizeUsed = sizeof(Config::DPI_SEGMENT_HEADER_t);
            m_localBufferSegments.emplace_back(remoteOffset, Config::DPI_SEGMENT_SIZE, 0);
            m_regClient->dpi_append_segment(m_handle->name, m_localBufferSegments.back());
        }
        BuffSegment &segment = m_localBufferSegments.back();

        if (!m_rdmaClient->write(m_handle->node_id, segment.offset + m_sizeUsed, m_scratchPad, size, true))
        {
            return false;
        }
        m_sizeUsed = m_sizeUsed + size;
        // update counter / header once in a while
        TO_BE_IMPLEMENTED(m_rdmaClient->write(HEADER));
        return true;
    }

    void *super_getScratchPad()
    {
        return m_scratchPad;
    }

  protected:
    BuffHandle *m_handle = nullptr;
    vector<BuffSegment> m_localBufferSegments;
    void *m_scratchPad = nullptr;
    size_t m_scratchPadSize = 0;

  private:
    size_t m_sizeUsed = 0;
    Config::DPI_SEGMENT_HEADER_t HEADER;
    RegistryClient *m_regClient = nullptr;
    RDMAClient *m_rdmaClient = nullptr; // used to issue RDMA req. to the NodeServer
};

} // namespace dpi