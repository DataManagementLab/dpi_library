/**
 * @file append_examples.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include "../../api/dpi.h"
#include "../../dpi/RegistryServer.h"
#include "../../dpi/NodeServer.h"

class AppendExamples : public CppUnit::TestFixture
{
  DPI_UNIT_TEST_SUITE(AppendExamples);
    DPI_UNIT_TEST(example1);
    DPI_UNIT_TEST(paperExample);
  DPI_UNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void example1();
  void paperExample();
 
private:
  RegistryServer *m_regServer;
  NodeServer *m_nodeServer;

};