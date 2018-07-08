#pragma once

#include "../../utils/Config.h"

#include "../../dpi/NodeServer.h"
#include "../../dpi/NodeClient.h"
#include "../../dpi/RegistryClient.h"
#include "../../dpi/BufferWriter.h"

class TestNodeClient : public CppUnit::TestFixture {
DPI_UNIT_TEST_SUITE(TestNodeClient);
  // DPI_UNIT_TEST(testAppendPrivate_WithScratchpad);
  // DPI_UNIT_TEST(testAppendPrivate_WithoutScratchpad);
  // DPI_UNIT_TEST(testAppendPrivate_MultipleClients_WithScratchpad);
  // DPI_UNIT_TEST(testAppendPrivate_SizeTooBigForScratchpad);
  // DPI_UNIT_TEST(testBuffer);
  // DPI_UNIT_TEST(testAppendPrivate_WithoutScratchpad_splitData);

  // DPI_UNIT_TEST(testAppendShared_AtomicHeaderManipulation);
  DPI_UNIT_TEST(testAppendShared_WithScratchpad);
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

};