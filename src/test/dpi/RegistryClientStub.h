/**
 * @file RegistryClientStub.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include "../../utils/Config.h"

#include "../../dpi/RegistryClient.h"
#include "../../dpi/BufferWriter.h"

// This class is only inteded for testing purposes!
class RegistryClientStub : public RegistryClient
{
public:

  RegistryClientStub(RDMAClient *rdmaClient) : m_rdmaClient(rdmaClient)
  {

  }
  // BufferHandle* createBuffer(string& name, NodeID node_id, size_t size, size_t threshold)
  // {
  //   (void) name;
  //   (void) size;
  //   (void) threshold;
    
  //   m_buffHandle = new BufferHandle(name, node_id, );
  //   RDMAClient *rdmaClient = new RDMAClient();
  //   rdmaClient->connect(Config::getIPFromNodeId(node_id));
  //   size_t remoteOffset = 0;
  //   rdmaClient->remoteAlloc(Config::getIPFromNodeId(node_id), Config::DPI_SEGMENT_SIZE, remoteOffset);
    
  //   BufferSegment seg;
  //   seg.offset = remoteOffset;
  //   seg.size = Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t);
  //   seg.threshold = Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR;
  //   appendSegment(name, seg);
    
  //   // std::cout << "Created buffer" << '\n';
    
  //   return m_buffHandle;
  // }

  
  BufferHandle *createSegmentRingOnBuffer(string &name)
  {
    size_t offset = 0;
    string connection = Config::getIPFromNodeId(m_buffHandle->node_id);
    size_t fullSegmentSize = m_buffHandle->segmentSizes + sizeof(Config::DPI_SEGMENT_HEADER_t);

    if (!m_rdmaClient->remoteAllocSegments(connection, name, m_buffHandle->segmentsPerWriter, fullSegmentSize, m_buffHandle->reuseSegments, true, offset))
    {
        return nullptr;
    }

    Logging::debug(__FILE__, __LINE__, "Created ring with offset on entry segment: " + to_string(offset));

    m_buffHandle->entrySegments.emplace_back(offset, m_buffHandle->segmentSizes, m_buffHandle->segmentSizes + sizeof(Config::DPI_SEGMENT_HEADER_t));

    BufferHandle*  copy_buffHandle = new BufferHandle(name, m_buffHandle->node_id, m_buffHandle->segmentsPerWriter, m_buffHandle->reuseSegments, m_buffHandle->segmentSizes);
    copy_buffHandle->entrySegments.push_back(m_buffHandle->entrySegments.back()); //Only the newly created entrysegment into the returned buffer handle
    return copy_buffHandle;
  }

  bool registerBuffer(BufferHandle* handle)
  {
    // std::cout << "Register Buffer" << '\n';
    BufferHandle* copy_buffHandle = new BufferHandle(handle->name, handle->node_id, handle->segmentsPerWriter, handle->reuseSegments, handle->segmentSizes);
    for(auto segment : handle->entrySegments){
      copy_buffHandle->entrySegments.push_back(segment); 
    }
    m_buffHandle = copy_buffHandle;
    return true;
  }
  BufferHandle* retrieveBuffer(string& name)
  {
    // std::cout << "Retrieve Buffer" << '\n';
    (void) name;
    //Copy a new BufferHandle to emulate distributed setting (Or else one nodes changes to the BufferHandle would affect another nodes BufferHandle without retrieving the buffer first)
    BufferHandle*  copy_buffHandle = new BufferHandle(m_buffHandle->name, m_buffHandle->node_id, m_buffHandle->segmentsPerWriter, m_buffHandle->reuseSegments, m_buffHandle->segmentSizes);
    for(auto segment : m_buffHandle->entrySegments){
      copy_buffHandle->entrySegments.push_back(segment); 
    }
    return copy_buffHandle;
  }
  bool appendSegment(string& name, BufferSegment& segment)
  {
    // std::cout << "Appending segment to buffer" << '\n';
    //Implement locking if stub should support concurrent appending of segments.
    (void) name;
    appendSegMutex.lock();
    BufferSegment seg;
    seg.offset = segment.offset;
    seg.size = segment.size;
    seg.nextSegmentOffset = segment.nextSegmentOffset;
    m_buffHandle->entrySegments.push_back(seg);
    appendSegMutex.unlock();
    
    return true;
  }

private:
  BufferHandle* m_buffHandle = nullptr; //For this stub we just have one buffHandle
  std::mutex appendSegMutex;
  RDMAClient* m_rdmaClient = nullptr; 
};