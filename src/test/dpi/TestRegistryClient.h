/**
 * @file TestRegistryClient.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include "../../utils/Config.h"

#include "../../dpi/RegistryClient.h"
#include "../../dpi/RegistryServer.h"
#include "../../dpi/BufferHandle.h"
#include "../../dpi/NodeServer.h"

class TestRegistryClient : public CppUnit::TestFixture
{
  DPI_UNIT_TEST_SUITE(TestRegistryClient);
  DPI_UNIT_TEST(testRetrieveBuffer);
  DPI_UNIT_TEST(testRegisterBuffer);
  DPI_UNIT_TEST(testJoinBuffer);
  // DPI_UNIT_TEST(testcreateSegmentRingOnBuffer);
  DPI_UNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void testRetrieveBuffer();
  void testRegisterBuffer();
  void testJoinBuffer();
  // void testcreateSegmentRingOnBuffer();

private:
  RegistryClient *m_regClient;
  RegistryServer *m_regServer;
  NodeServer *m_nodeServer; 
};