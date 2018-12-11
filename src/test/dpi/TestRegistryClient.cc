#include "TestRegistryClient.h"

void TestRegistryClient::setUp()
{
  Config::RDMA_MEMSIZE = 1024ul * 1024 * 1024 * 1; //1GB
  Config::DPI_SEGMENT_SIZE = (2048 + sizeof(Config::DPI_SEGMENT_HEADER_t));
  Config::DPI_INTERNAL_BUFFER_SIZE = 1024;
  Config::DPI_REGISTRY_SERVER = "127.0.0.1";
  Config::DPI_NODES.clear();
  string dpiTestNode = "127.0.0.1:" + to_string(Config::DPI_NODE_PORT);
  Config::DPI_NODES.push_back(dpiTestNode);
  m_nodeServer = new NodeServer();
  CPPUNIT_ASSERT(m_nodeServer->startServer());
  std::cout << "Start NodeServer" << '\n';
  m_regServer = new RegistryServer();
  std::cout << "Start RegServer" << '\n';
  CPPUNIT_ASSERT(m_regServer->startServer());
  std::cout << "Started NodeServer" << '\n';

  m_regClient = new RegistryClient();
  std::cout << "Start RegClient" << '\n';
}

void TestRegistryClient::tearDown()
{
  if (m_regServer != nullptr)
  {
    m_regServer->stopServer();
    delete m_regServer;
    m_regServer = nullptr;
  }
  if (m_nodeServer != nullptr)
  {
    m_nodeServer->stopServer();
    delete m_nodeServer;
    m_nodeServer = nullptr;
  }
  if (m_regClient != nullptr)
  {
    delete m_regClient;
    m_regClient = nullptr;
  }
}

void TestRegistryClient::testRetrieveBuffer()
{
  string name = "buffer2";
  CPPUNIT_ASSERT(m_regClient->registerBuffer(new BufferHandle(name, 1, 1, false, 1024)));

  auto buffHandle = m_regClient->retrieveBuffer(name);
  CPPUNIT_ASSERT(buffHandle != nullptr);
  CPPUNIT_ASSERT_EQUAL(buffHandle->name, name);

  CPPUNIT_ASSERT_EQUAL(0, ((int)buffHandle->entrySegments.size()));
};

void TestRegistryClient::testRegisterBuffer()
{
  string name = "buffer1";
  BufferHandle *buffHandle = new BufferHandle(name,1,1);
  CPPUNIT_ASSERT( m_regClient->registerBuffer(buffHandle));

  BufferHandle* handle_ret = m_regClient->retrieveBuffer(name);
  CPPUNIT_ASSERT(handle_ret != nullptr);
  CPPUNIT_ASSERT_EQUAL(handle_ret->name, name);
  CPPUNIT_ASSERT_EQUAL(0, ((int)handle_ret->entrySegments.size()));
};

void TestRegistryClient::testcreateSegmentRingOnBuffer()
{
  string name = "buffer1";
  BufferHandle *buffHandle = new BufferHandle(name, 1, 1, false, 1024);
  CPPUNIT_ASSERT(m_regClient->registerBuffer(buffHandle));

  BufferHandle* handle_ret = m_regClient->createSegmentRingOnBuffer(name);
  std::cout << "Received buffer handle from createSegmentRingOnBuffer()" << '\n';
  CPPUNIT_ASSERT(handle_ret != nullptr);
  CPPUNIT_ASSERT_EQUAL(handle_ret->name, name);
  CPPUNIT_ASSERT_EQUAL(1, ((int)handle_ret->entrySegments.size()));  
  BufferSegment segment = handle_ret->entrySegments[0];
  CPPUNIT_ASSERT_EQUAL(1024, ((int)segment.size));
};
