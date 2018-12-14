/**
 * @file TestBufferReader.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include "../../utils/Config.h"

#include "../../dpi/NodeServer.h" 
#include "../../dpi/RegistryClient.h"
#include "../../dpi/RegistryServer.h"
#include "../../dpi/BufferWriter.h"
#include "../../dpi/BufferReader.h"
#include "RegistryClientStub.h"

#include <atomic>

class TestBufferReader : public CppUnit::TestFixture {
DPI_UNIT_TEST_SUITE(TestBufferReader);
  DPI_UNIT_TEST(testReadWithHeader);
  // DPI_UNIT_TEST(testReadWithoutHeader);
DPI_UNIT_TEST_SUITE_END();
 
 public:
  void setUp();
  void tearDown();
 
  void testReadWithHeader();
  void testReadWithoutHeader();

private:
  NodeServer* m_nodeServer;
  RegistryClient* m_regClient;
  RegistryServer* m_regServer;
};
