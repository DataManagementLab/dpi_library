#include "TestRegistryClient.h"

void TestRegistryClient::setUp()
{
  m_nodeServer = new NodeServer();
  CPPUNIT_ASSERT(m_nodeServer->startServer());

  m_regServer = new RegistryServer();
  CPPUNIT_ASSERT(m_regServer->startServer());

  m_regClient = new RegistryClient();
}

void TestRegistryClient::tearDown()
{
  if (m_regServer != nullptr) {
    m_regServer->stopServer();
    delete m_regServer;
    m_regServer = nullptr;
  }
  if (m_nodeServer != nullptr) {
    m_nodeServer->stopServer();
    delete m_nodeServer;
    m_nodeServer = nullptr;
  }
  if (m_regClient != nullptr) {
    delete m_regClient;
    m_regClient = nullptr;
  }
}

void TestRegistryClient::testCreateBuffer()
{
  string name = "buffer1";
  BuffHandle *buffHandle = m_regClient->dpi_create_buffer(name, 0, 1024, 800);

  CPPUNIT_ASSERT(buffHandle!=nullptr);
  CPPUNIT_ASSERT_EQUAL(buffHandle->name, name);

  CPPUNIT_ASSERT_EQUAL(1, ((int)buffHandle->segments.size()));
  BuffSegment segment = buffHandle->segments[0];
  CPPUNIT_ASSERT_EQUAL(1024, ((int)segment.size));
  CPPUNIT_ASSERT_EQUAL(800, ((int)segment.threshold));
}

void TestRegistryClient::testRetrieveBuffer()
{
  string name = "buffer2";
  BuffHandle *buffHandle = m_regClient->dpi_create_buffer(name, 0, 1024, 800);
  CPPUNIT_ASSERT(buffHandle!=nullptr);

  buffHandle = m_regClient->dpi_retrieve_buffer(name);
  CPPUNIT_ASSERT(buffHandle!=nullptr);
  CPPUNIT_ASSERT_EQUAL(buffHandle->name, name);

  CPPUNIT_ASSERT_EQUAL(1, ((int)buffHandle->segments.size()));
  BuffSegment segment = buffHandle->segments[0];
  CPPUNIT_ASSERT_EQUAL(1024, ((int)segment.size));
  CPPUNIT_ASSERT_EQUAL(800, ((int)segment.threshold));
};