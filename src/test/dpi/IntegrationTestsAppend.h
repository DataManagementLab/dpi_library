#pragma once

#include "../../utils/Config.h"

#include "../../dpi/RegistryClient.h"
#include "../../dpi/RegistryServer.h"
#include "../../dpi/BufferHandle.h"
#include "../../dpi/NodeServer.h"
#include "../../dpi/NodeClient.h"
#include "../../dpi/RegistryClient.h"
#include "../../dpi/BufferWriter.h"

class IntegrationTestsAppend : public CppUnit::TestFixture
{
  DPI_UNIT_TEST_SUITE(IntegrationTestsAppend);
  DPI_UNIT_TEST(testPrivate_SimpleIntegrationWithAppendInts);
  DPI_UNIT_TEST(testShared_SimpleIntegrationWithAppendInts);
  DPI_UNIT_TEST(testShared_AtomicHeaderManipulation);
  DPI_UNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void testPrivate_SimpleIntegrationWithAppendInts();
  void testShared_SimpleIntegrationWithAppendInts();
  void testShared_AtomicHeaderManipulation();

private:
  RegistryClient *m_regClient;
  RegistryServer *m_regServer;
  NodeServer *m_nodeServer; 
  NodeClient *m_nodeClient; 
};