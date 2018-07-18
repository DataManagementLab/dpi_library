#include "IntegrationTestsAppend.h"

void IntegrationTestsAppend::setUp()
{  
  //Setup Test DPI
  Config::RDMA_MEMSIZE = 1024ul * 1024 * 1024 * 1;  //1GB
  Config::DPI_SEGMENT_SIZE = (2048 + sizeof(Config::DPI_SEGMENT_HEADER_t));
  Config::DPI_INTERNAL_BUFFER_SIZE = 1024;
  Config::DPI_REGISTRY_SERVER = "127.0.0.1";
  Config::DPI_NODES.clear();
  string dpiTestNode = "127.0.0.1:" + to_string(Config::DPI_NODE_PORT);
  Config::DPI_NODES.push_back(dpiTestNode);
  Config::DPI_REGISTRY_SERVER = "127.0.0.1";

  m_nodeServer = new NodeServer();
  CPPUNIT_ASSERT(m_nodeServer->startServer());
  std::cout << "Start NodeServer" << '\n';
  m_nodeClient = new NodeClient();
  std::cout << "Start NodeClient" << '\n';
  m_regServer = new RegistryServer();
  CPPUNIT_ASSERT(m_regServer->startServer());
  std::cout << "Start RegServer" << '\n';
  m_regClient = new RegistryClient();
  std::cout << "Start RegClient" << '\n';
}

void IntegrationTestsAppend::tearDown()
{
  if (m_nodeClient != nullptr)
  {
    delete m_nodeClient;
    m_nodeClient = nullptr;
  }
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

void IntegrationTestsAppend::testPrivate_SimpleIntegrationWithAppendInts()
{
  //ARRANGE
  string bufferName = "buffer1";
    
  size_t remoteOffset = 0;
  size_t memSize = sizeof(int);

  uint32_t numberSegments = 2;
  size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / memSize * numberSegments;

  BufferHandle *buffHandle = new BufferHandle(bufferName, 1);
  BufferWriter<BufferWriterPrivate> buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_regClient);
  m_regClient->registerBuffer(buffHandle);

  //ACT
  for (int i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter, (void *)&i, memSize));
  }

  CPPUNIT_ASSERT(buffWriter.close());
  int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);
  
  for(size_t i = 0; i < numberSegments*Config::DPI_SEGMENT_SIZE/memSize; i++)
  {
    std::cout << rdma_buffer[i] << " ";
  }
  
  //ASSERT
  for (uint32_t j = 0; j < numberSegments; j++)
  {
    //Assert header
    Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *) &(rdma_buffer[j*Config::DPI_SEGMENT_SIZE / memSize]);
    CPPUNIT_ASSERT_EQUAL((uint64_t)(Config::DPI_SEGMENT_SIZE- sizeof(Config::DPI_SEGMENT_HEADER_t)), header[0].counter);
    CPPUNIT_ASSERT_EQUAL((uint64_t)(j == numberSegments - 1 ? 0 : 1), header[0].hasFollowSegment);

    //Assert data
    int expected = 0;
    for (uint32_t i = sizeof(Config::DPI_SEGMENT_HEADER_t) / memSize; i < (numberElements / numberSegments); i++)
    {
      CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[i]);
      expected++;
    }
  }
}

void IntegrationTestsAppend::testShared_SimpleIntegrationWithAppendInts()
{
  //ARRANGE
  string bufferName = "buffer1";
    
  size_t remoteOffset = 0;
  size_t memSize = sizeof(int);

  uint32_t numberSegments = 2;
  size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / memSize * numberSegments;

  BufferHandle *buffHandle = m_regClient->createBuffer(bufferName, 1, (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)),Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR);
  BufferWriter<BufferWriterShared> buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_regClient);

  //ACT
  for (int i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter, (void *)&i, memSize));
  }

  CPPUNIT_ASSERT(buffWriter.close());
  int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);
  
  
  for(size_t i = 0; i < numberSegments*Config::DPI_SEGMENT_SIZE/memSize; i++)
  {
    std::cout << rdma_buffer[i] << " ";
  }
  

  //ASSERT
  for (uint32_t j = 0; j < numberSegments; j++)
  {
    //Assert header
    Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *) &(rdma_buffer[j*Config::DPI_SEGMENT_SIZE / memSize]);
    CPPUNIT_ASSERT_EQUAL((uint64_t)(Config::DPI_SEGMENT_SIZE- sizeof(Config::DPI_SEGMENT_HEADER_t)), header[0].counter);
    CPPUNIT_ASSERT_EQUAL((uint64_t)1, header[0].hasFollowSegment);

    //Assert data
    int expected = 0;
    for (uint32_t i = sizeof(Config::DPI_SEGMENT_HEADER_t) / memSize; i < (numberElements / numberSegments); i++)
    {
      CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[i]);
      expected++;
    }
  }
}

void IntegrationTestsAppend::testShared_AtomicHeaderManipulation()
{

  //ARRANGE
  string bufferName = "test";
  NodeID nodeId = 1;

  uint64_t expected = 10;
  uint64_t fetched = 0;

  BufferHandle *buffHandle = m_regClient->createBuffer(bufferName, nodeId, (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)),Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR);
  BufferWriter<BufferWriterShared> buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_regClient);

  //ACT
  auto fetchedCounter = buffWriter.modifyCounter(expected, 0 + Config::DPI_SEGMENT_HEADER_META::getCounterOffset);

  //ASSERT
  Config::DPI_SEGMENT_HEADER_t *rdma_buffer = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(0);
  CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[0].counter);
  CPPUNIT_ASSERT_EQUAL(fetched, fetchedCounter);
  CPPUNIT_ASSERT_EQUAL((uint64_t)0, rdma_buffer[0].hasFollowSegment);


  //ACT 2
  int64_t subtract = -5;
  fetchedCounter = buffWriter.modifyCounter(subtract, 0 + Config::DPI_SEGMENT_HEADER_META::getCounterOffset);

  //ASSERT 2
  rdma_buffer = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(0);
  CPPUNIT_ASSERT_EQUAL((uint64_t)((int64_t)expected + subtract), rdma_buffer[0].counter);
  CPPUNIT_ASSERT_EQUAL(expected, fetchedCounter);
  CPPUNIT_ASSERT_EQUAL((uint64_t)0, rdma_buffer[0].hasFollowSegment);

  //ACT 3
  auto fetchedFollowPage = buffWriter.setHasFollowSegment(0);

  //ASSERT 3
  CPPUNIT_ASSERT_EQUAL((uint64_t)1, rdma_buffer[0].hasFollowSegment);
  CPPUNIT_ASSERT_EQUAL(fetched, fetchedFollowPage);

  //ACT 4
  fetchedFollowPage = buffWriter.setHasFollowSegment(0);

  //ASSERT 4
  CPPUNIT_ASSERT_EQUAL((uint64_t)1, rdma_buffer[0].hasFollowSegment);
  CPPUNIT_ASSERT_EQUAL((uint64_t)1, fetchedFollowPage);

  //Allocate new segment
  RDMAClient *m_rdmaClient = new RDMAClient();
  m_rdmaClient->connect(Config::getIPFromNodeId(nodeId), nodeId);

  size_t remoteOffset = 0;
  CPPUNIT_ASSERT(m_rdmaClient->remoteAlloc(Config::getIPFromNodeId(nodeId), Config::DPI_SEGMENT_SIZE, remoteOffset));
  std::cout << "Allocated new segment" << '\n';

  //ACT 5
  fetchedCounter = buffWriter.modifyCounter(expected, remoteOffset + Config::DPI_SEGMENT_HEADER_META::getCounterOffset);

  //ASSERT 5
  rdma_buffer = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(remoteOffset);
  CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[0].counter);
  CPPUNIT_ASSERT_EQUAL(fetched, fetchedCounter);
  CPPUNIT_ASSERT_EQUAL((uint64_t)0, rdma_buffer[0].hasFollowSegment);

  //ACT 6
  subtract = -5;
  fetchedCounter = buffWriter.modifyCounter(subtract, remoteOffset);

  //ASSERT 6
  rdma_buffer = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(remoteOffset);
  CPPUNIT_ASSERT_EQUAL((uint64_t)((int64_t)expected + subtract), rdma_buffer[0].counter);
  CPPUNIT_ASSERT_EQUAL(expected, fetchedCounter);
  CPPUNIT_ASSERT_EQUAL((uint64_t)0, rdma_buffer[0].hasFollowSegment);

  CPPUNIT_ASSERT(buffWriter.close());
}