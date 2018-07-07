#pragma once

#include "../../utils/Config.h"

#include "../../dpi/NodeServer.h"
#include "../../dpi/NodeClient.h"
#include "../../dpi/RegistryClient.h"
#include "../../dpi/BufferWriter.h"

class TestNodeClient : public CppUnit::TestFixture {
DPI_UNIT_TEST_SUITE(TestNodeClient);
  DPI_UNIT_TEST(testAppendShared_WithScratchpad);
  // DPI_UNIT_TEST(testAppendShared_WithoutScratchpad);
  // DPI_UNIT_TEST(testBuffer);
  // DPI_UNIT_TEST(testRemoteAlloc);
  DPI_UNIT_TEST_SUITE_END()
  ;

 public:
  void setUp();
  void tearDown();
  void testAppendShared_WithScratchpad();
  void testAppendShared_WithoutScratchpad();
  void testRemoteAlloc();
  void testBuffer();

private:
  NodeClient* m_nodeClient;
  NodeServer* m_nodeServer;
  RegistryClient* m_regClient;
};