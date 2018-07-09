#pragma once

#include "../../utils/Config.h"

#include "../../dpi/NodeServer.h"
#include "../../dpi/NodeClient.h"
#include "../../dpi/RegistryClient.h"
#include "../../dpi/BufferWriter.h"

class TestBufferWriter : public CppUnit::TestFixture {
DPI_UNIT_TEST_SUITE(TestBufferWriter);
  DPI_UNIT_TEST(testAppendPrivate_WithScratchpad);
  DPI_UNIT_TEST(testAppendPrivate_WithoutScratchpad);
  DPI_UNIT_TEST(testAppendPrivate_MultipleClients_WithScratchpad);
  DPI_UNIT_TEST(testAppendPrivate_SizeTooBigForScratchpad);
  DPI_UNIT_TEST(testBuffer);
  DPI_UNIT_TEST(testAppendPrivate_WithoutScratchpad_splitData);

  DPI_UNIT_TEST(testAppendShared_AtomicHeaderManipulation);
  DPI_UNIT_TEST(testAppendShared_MultipleConcurrentClients);  
DPI_UNIT_TEST_SUITE_END();

 public:
  void setUp();
  void tearDown();

  // Private Strategy
  void testAppendPrivate_WithScratchpad();
  void testAppendPrivate_WithoutScratchpad_splitData();
  void testAppendPrivate_WithoutScratchpad();
  void testAppendPrivate_MultipleClients_WithScratchpad();
  void testAppendPrivate_SizeTooBigForScratchpad();
  void testBuffer();

  // Shared Strategy
  void testAppendShared_AtomicHeaderManipulation();
  void testAppendShared_WithScratchpad();
  void testAppendShared_MultipleConcurrentClients();

private:
  NodeClient* m_nodeClient;
  NodeServer* m_nodeServer;
  RegistryClient* m_stub_regClient;


class RegistryClientStub : public RegistryClient
{
public:

  BuffHandle* dpi_create_buffer(string& name, NodeID node_id, string& connection)
  {
    std::cout << "Created buffer" << '\n';
    (void) name;
    m_buffHandle = new BuffHandle(name, node_id, connection);
    return m_buffHandle;
  }

  bool dpi_register_buffer(BuffHandle* handle)
  {
    m_buffHandle = handle;
    return true;
  }
  BuffHandle* dpi_retrieve_buffer(string& name)
  {
    (void) name;
    BuffHandle*  copy_buffHandle = new BuffHandle(m_buffHandle->name, m_buffHandle->node_id, m_buffHandle->connection);
    for(auto segment : m_buffHandle->segments){
      copy_buffHandle->segments.push_back(segment);
    }
    return copy_buffHandle;
  }
  bool dpi_append_segment(string& name, BuffSegment& segment)
  {
    //Implement locking if stub should support concurrent appending of segments.
    (void) name;
    BuffSegment seg;
    seg.offset = segment.offset;
    seg.size = segment.size;
    seg.threshold = segment.threshold;
    m_buffHandle->segments.push_back(seg);
    return true;
  }

private:
  BuffHandle* m_buffHandle = nullptr; //For this stub we just have one buffHandle
};
 
class BufferWriterSharedClient : public Thread
{
  NodeServer* nodeServer = nullptr;
  RegistryClient* regClient = nullptr;
  std::vector<std::tuple<int*, int>> *dataToWrite = nullptr; //tuple<ptr to data, size in bytes>

public: 
  BufferWriterSharedClient(NodeServer* nodeServer, RegistryClient* regClient, std::vector<std::tuple<int*, int>> *dataToWrite) : 
    Thread(), nodeServer(nodeServer), regClient(regClient), dataToWrite(dataToWrite) {}

  virtual void run() 
  {
    //ARRANGE
    string bufferName = "test";
    string connection = "127.0.0.1:5400";

    BuffHandle *buffHandle = new BuffHandle(bufferName, 1, connection);
    BufferWriter<BufferWriterShared> buffWriter(buffHandle, Config::DPI_SCRATCH_PAD_SIZE, regClient);

    //ACT
    for(int i = 0; i < dataToWrite->size(); i++)
    {
      std::cout << "Size: " << std::get<1>(dataToWrite->operator[](i)) << '\n';
      buffWriter.append(std::get<0>(dataToWrite->operator[](i)), std::get<1>(dataToWrite->operator[](i))*sizeof(int));
    }
  }
};

};