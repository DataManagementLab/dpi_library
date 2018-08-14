#pragma once

#include "../../utils/Config.h"

#include "../../dpi/RegistryClient.h"
#include "../../dpi/BufferWriter.h"

// This class is only inteded for testing purposes!
class RegistryClientStub : public RegistryClient
{
public:

  BufferHandle* createBuffer(string& name, NodeID node_id, size_t size, size_t threshold)
  {
    (void) name;
    (void) size;
    (void) threshold;
    
    m_buffHandle = new BufferHandle(name, node_id);
    RDMAClient *rdmaClient = new RDMAClient();
    rdmaClient->connect(Config::getIPFromNodeId(node_id));
    size_t remoteOffset = 0;
    rdmaClient->remoteAlloc(Config::getIPFromNodeId(node_id), Config::DPI_SEGMENT_SIZE, remoteOffset);
    
    BufferSegment seg;
    seg.offset = remoteOffset;
    seg.size = Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t);
    seg.threshold = Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR;
    appendSegment(name, seg);
    
    // std::cout << "Created buffer" << '\n';
    
    return m_buffHandle;
  }

  bool registerBuffer(BufferHandle* handle)
  {
    // std::cout << "Register Buffer" << '\n';
    BufferHandle* copy_buffHandle = new BufferHandle(handle->name, handle->node_id);
    for(auto segment : handle->segments){
      copy_buffHandle->segments.push_back(segment); 
    }
    m_buffHandle = copy_buffHandle;
    return true;
  }
  BufferHandle* retrieveBuffer(string& name)
  {
    // std::cout << "Retrieve Buffer" << '\n';
    (void) name;
    //Copy a new BufferHandle to emulate distributed setting (Or else one nodes changes to the BufferHandle would affect another nodes BufferHandle without retrieving the buffer first)
    BufferHandle*  copy_buffHandle = new BufferHandle(m_buffHandle->name, m_buffHandle->node_id);
    for(auto segment : m_buffHandle->segments){
      copy_buffHandle->segments.push_back(segment); 
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
    seg.threshold = segment.threshold;
    m_buffHandle->segments.push_back(seg);
    appendSegMutex.unlock();
    
    return true;
  }

private:
  BufferHandle* m_buffHandle = nullptr; //For this stub we just have one buffHandle
  std::mutex appendSegMutex;
};