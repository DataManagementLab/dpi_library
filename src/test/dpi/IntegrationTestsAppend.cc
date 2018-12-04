#include "IntegrationTestsAppend.h"

std::atomic<int> IntegrationTestsAppend::bar{0};    // Counter of threads, faced barrier.
std::atomic<int> IntegrationTestsAppend::passed{0}; // Number of barriers, passed by all threads.

void IntegrationTestsAppend::setUp()
{  
  //Setup Test DPI
  Config::RDMA_MEMSIZE = 1024ul * 1024 * 1024 * 1;  //1GB
  Config::DPI_SEGMENT_SIZE = (2048 + sizeof(Config::DPI_SEGMENT_HEADER_t));
  Config::DPI_INTERNAL_BUFFER_SIZE = 1024;
  Config::DPI_REGISTRY_SERVER = "127.0.0.1";
  Config::DPI_REGISTRY_PORT = 5300;
  Config::DPI_NODE_PORT = 5400;
  Config::DPI_NODES.clear();
  string dpiTestNode = "127.0.0.1:" + to_string(Config::DPI_NODE_PORT);
  Config::DPI_NODES.push_back(dpiTestNode);

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

void IntegrationTestsAppend::SimpleIntegrationWithAppendInts_DontReuseSegs()
{
  //ARRANGE
  string bufferName = "buffer1";
    
  size_t remoteOffset = 0;
  size_t memSize = sizeof(int);

  uint32_t numberSegments = 2;
  size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / memSize * numberSegments;
  uint32_t segmentsPerWriter = 2;
  BufferHandle *buffHandle = new BufferHandle(bufferName, 1, segmentsPerWriter);
  DPI_DEBUG("Created BufferHandle\n");
  m_regClient->registerBuffer(buffHandle);
  DPI_DEBUG("Registered Buffer in Registry\n");
  buffHandle = m_regClient->createSegmentRingOnBuffer(bufferName);
  DPI_DEBUG("Created segment ring on buffer\n");


  int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);
  for(size_t i = 0; i < numberSegments*Config::DPI_SEGMENT_SIZE/memSize; i++)
  {
    std::cout << rdma_buffer[i] << " ";
  }

  BufferWriter buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_regClient);

  //ACT
  for (size_t i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter, (void *)&i, memSize));
  }

  CPPUNIT_ASSERT(buffWriter.close());
  
  for(size_t i = 0; i < numberSegments*Config::DPI_SEGMENT_SIZE/memSize; i++)
  {
    std::cout << rdma_buffer[i] << " ";
  }
  
  //ASSERT
  for (uint32_t j = 0; j < numberSegments; j++)
  {
    //Assert header
    Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *) &(rdma_buffer[j*Config::DPI_SEGMENT_SIZE / memSize]);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected size of segment (without header) did not match segment counter", (uint64_t)(Config::DPI_SEGMENT_SIZE- sizeof(Config::DPI_SEGMENT_HEADER_t)), header[0].counter);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("hasFollowSegment did not match expected", (uint64_t)(j == numberSegments - 1 ? 0 : 1), header[0].hasFollowSegment);

    //Assert data
    int expected = 0;
    for (uint32_t i = sizeof(Config::DPI_SEGMENT_HEADER_t) / memSize; i < (numberElements / numberSegments); i++)
    {
      CPPUNIT_ASSERT_EQUAL_MESSAGE("Data value did not match", expected, rdma_buffer[i]);
      expected++;
    }
  }
}


void IntegrationTestsAppend::FourAppendersConcurrent_DontReuseSegs()
{
  //ARRANGE
  string bufferName = "test";
  int nodeId = 1;

  size_t segPerClient = 2;
  int64_t numberElements = segPerClient * (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int64_t); //Each client fill two segments
  std::vector<int64_t> *dataToWrite = new std::vector<int64_t>();
  
  for(int64_t i = 0; i < numberElements; i++)
  {
    dataToWrite->push_back(i);
  }  

  CPPUNIT_ASSERT(m_regClient->registerBuffer(new BufferHandle(bufferName, nodeId, segPerClient)));
  BufferHandle *buffHandle1 = m_regClient->createSegmentRingOnBuffer(bufferName);
  BufferHandle *buffHandle2 = m_regClient->createSegmentRingOnBuffer(bufferName);
  BufferHandle *buffHandle3 = m_regClient->createSegmentRingOnBuffer(bufferName);
  BufferHandle *buffHandle4 = m_regClient->createSegmentRingOnBuffer(bufferName);

  int64_t *rdma_buffer = (int64_t *)m_nodeServer->getBuffer();

  // std::cout << "Buffer before appending" << '\n';
  // for (int i = 0; i < Config::DPI_SEGMENT_SIZE/sizeof(int64_t)*segPerClient*4; i++)
  // {
  //   std::cout << rdma_buffer[i] << ' ';
  // }

  BufferWriterClient<int64_t> *client1 = new BufferWriterClient<int64_t>(buffHandle1, dataToWrite);
  BufferWriterClient<int64_t> *client2 = new BufferWriterClient<int64_t>(buffHandle2, dataToWrite);
  BufferWriterClient<int64_t> *client3 = new BufferWriterClient<int64_t>(buffHandle3, dataToWrite);
  BufferWriterClient<int64_t> *client4 = new BufferWriterClient<int64_t>(buffHandle4, dataToWrite);

  //ACT
  client1->start();
  client2->start();
  client3->start();
  client4->start();
  client1->join();
  client2->join();
  client3->join();
  client4->join();
  
  //ASSERT

  // std::cout << "Buffer after appending, total int64_t's: " << numberElements << " * 4" << '\n';
  // for (int i = 0; i < Config::DPI_SEGMENT_SIZE/sizeof(int64_t)*segPerClient*4; i++)
  // {
  //   std::cout << rdma_buffer[i] << ' ';
  // }

  for(size_t i = 0; i < segPerClient*4; i++)
  {
    int64_t segmentStart = i*Config::DPI_SEGMENT_SIZE/sizeof(int64_t);
    //Assert counter in header
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Counter in header check", (int64_t)(sizeof(int64_t)*numberElements/segPerClient), rdma_buffer[segmentStart]);
    if (rdma_buffer[segmentStart+1] == (int64_t)1) //Has follow segment, data should be dataToWrite[0 -> numberElements/2]
    {
      for(int64_t j = segmentStart; j < (int64_t)(segmentStart + numberElements/segPerClient); j++)
      {
        CPPUNIT_ASSERT_EQUAL(dataToWrite->operator[](j - segmentStart), rdma_buffer[j+sizeof(Config::DPI_SEGMENT_HEADER_t)/sizeof(int64_t)]);
      }      
    }
    else //data should be dataToWrite[numberElements/2 -> numberElements]
    {
      for(int64_t j = segmentStart; j < (int64_t)(segmentStart + numberElements/segPerClient); j++)
      {
        CPPUNIT_ASSERT_EQUAL(dataToWrite->operator[](j+numberElements/segPerClient - segmentStart), rdma_buffer[j+sizeof(Config::DPI_SEGMENT_HEADER_t)/sizeof(int64_t)]);
      }
    }
  }
  m_nodeServer->localFree(rdma_buffer);
}


// void IntegrationTestsAppend::testShared_SimpleIntegrationWithAppendInts_DontReuseSegs()
// {
//   //ARRANGE
//   string bufferName = "buffer1";
    
//   size_t remoteOffset = 0;
//   size_t memSize = sizeof(int);

//   uint32_t numberSegments = 2;
//   size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / memSize * numberSegments;

//   BufferHandle *buffHandle = m_regClient->createBuffer(bufferName, 1, (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)),Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR);
//   BufferWriter<BufferWriterShared> buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_regClient);

//   //ACT
//   for (size_t i = 0; i < numberElements; i++)
//   {
//     CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter, (void *)&i, memSize));
//   }

//   CPPUNIT_ASSERT(buffWriter.close());
//   int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);
  
  
//   // for(size_t i = 0; i < numberSegments*Config::DPI_SEGMENT_SIZE/memSize; i++)
//   // {
//   //   std::cout << rdma_buffer[i] << " ";
//   // }
  

//   //ASSERT
//   for (uint32_t j = 0; j < numberSegments; j++)
//   {
//     //Assert header
//     Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *) &(rdma_buffer[j*Config::DPI_SEGMENT_SIZE / memSize]);
//     CPPUNIT_ASSERT_EQUAL((uint64_t)(Config::DPI_SEGMENT_SIZE- sizeof(Config::DPI_SEGMENT_HEADER_t)), header[0].counter);
//     CPPUNIT_ASSERT_EQUAL((uint64_t)1, header[0].hasFollowSegment);

//     //Assert data
//     int expected = 0;
//     for (uint32_t i = sizeof(Config::DPI_SEGMENT_HEADER_t) / memSize; i < (numberElements / numberSegments); i++)
//     {
//       CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[i]);
//       expected++;
//     }
//   }
// }

// void IntegrationTestsAppend::testShared_FourAppendersConcurrent_DontReuseSegs()
// {
//   //ARRANGE
//   string bufferName = "test";
//   int nodeId = 1;

//   size_t segPerClient = 2; //Each client fill two segments
//   int64_t numberElements = segPerClient * (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int64_t); 
//   std::vector<int64_t> *dataToWrite = new std::vector<int64_t>();
  
//   for(int64_t i = 0; i < numberElements; i++)
//   {
//     dataToWrite->push_back(i);
//   }  

//   BufferHandle *buffHandle1 = m_regClient->createBuffer(bufferName, (NodeID) nodeId, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t), Config::DPI_SEGMENT_SIZE * 0.8);
//   BufferHandle *buffHandle2 = m_regClient->retrieveBuffer(bufferName);
//   BufferHandle *buffHandle3 = m_regClient->retrieveBuffer(bufferName);
//   BufferHandle *buffHandle4 = m_regClient->retrieveBuffer(bufferName);

//   BufferWriterClient<int64_t, BufferWriterShared> *client1 = new BufferWriterClient<int64_t, BufferWriterShared>(buffHandle1, dataToWrite);
//   BufferWriterClient<int64_t, BufferWriterShared> *client2 = new BufferWriterClient<int64_t, BufferWriterShared>(buffHandle2, dataToWrite);
//   BufferWriterClient<int64_t, BufferWriterShared> *client3 = new BufferWriterClient<int64_t, BufferWriterShared>(buffHandle3, dataToWrite);
//   BufferWriterClient<int64_t, BufferWriterShared> *client4 = new BufferWriterClient<int64_t, BufferWriterShared>(buffHandle4, dataToWrite);

//   //ACT
//   client1->start();
//   client2->start();
//   client3->start();
//   client4->start();
//   client1->join();
//   client2->join();
//   client3->join();
//   client4->join();
  
//   //ASSERT
//   BufferReader bufferReader(buffHandle1, m_regClient);
//   size_t read_buffer_size = 0;
//   int64_t *read_buffer = (int64_t *)bufferReader.read(read_buffer_size, true);

//   for(size_t i = 0; i < segPerClient*4; i++)
//   {
//     int64_t segmentStart = i*Config::DPI_SEGMENT_SIZE/sizeof(int64_t);
//     //Assert counter in header
//     CPPUNIT_ASSERT_EQUAL_MESSAGE("Counter in header check", (int64_t)(sizeof(int64_t)*numberElements/segPerClient), read_buffer[segmentStart]);
//     CPPUNIT_ASSERT_EQUAL_MESSAGE("Has follow segment in header check", (int64_t)1, read_buffer[segmentStart+1]);
//   }
//   m_nodeServer->localFree(read_buffer);
// }

// void IntegrationTestsAppend::testShared_AtomicHeaderManipulation()
// {

//   //ARRANGE
//   string bufferName = "test";
//   NodeID nodeId = 1;

//   uint64_t expected = 10;
//   uint64_t fetched = 0;

//   BufferHandle *buffHandle = m_regClient->createBuffer(bufferName, nodeId, (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)),Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR);
//   BufferWriter<BufferWriterShared> buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_regClient);

//   //ACT
//   auto fetchedCounter = buffWriter.modifyCounter(expected, 0 + Config::DPI_SEGMENT_HEADER_META::getCounterOffset);

//   //ASSERT
//   Config::DPI_SEGMENT_HEADER_t *rdma_buffer = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(0);
//   CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[0].counter);
//   CPPUNIT_ASSERT_EQUAL(fetched, fetchedCounter);
//   CPPUNIT_ASSERT_EQUAL((uint64_t)0, rdma_buffer[0].hasFollowSegment);


//   //ACT 2
//   int64_t subtract = -5;
//   fetchedCounter = buffWriter.modifyCounter(subtract, 0 + Config::DPI_SEGMENT_HEADER_META::getCounterOffset);

//   //ASSERT 2
//   rdma_buffer = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(0);
//   CPPUNIT_ASSERT_EQUAL((uint64_t)((int64_t)expected + subtract), rdma_buffer[0].counter);
//   CPPUNIT_ASSERT_EQUAL(expected, fetchedCounter);
//   CPPUNIT_ASSERT_EQUAL((uint64_t)0, rdma_buffer[0].hasFollowSegment);

//   //ACT 3
//   auto fetchedFollowPage = buffWriter.setHasFollowSegment(0);

//   //ASSERT 3
//   CPPUNIT_ASSERT_EQUAL((uint64_t)1, rdma_buffer[0].hasFollowSegment);
//   CPPUNIT_ASSERT_EQUAL(fetched, fetchedFollowPage);

//   //ACT 4
//   fetchedFollowPage = buffWriter.setHasFollowSegment(0);

//   //ASSERT 4
//   CPPUNIT_ASSERT_EQUAL((uint64_t)1, rdma_buffer[0].hasFollowSegment);
//   CPPUNIT_ASSERT_EQUAL((uint64_t)1, fetchedFollowPage);

//   //Allocate new segment
//   RDMAClient *m_rdmaClient = new RDMAClient();
//   m_rdmaClient->connect(Config::getIPFromNodeId(nodeId), nodeId);

//   size_t remoteOffset = 0;
//   CPPUNIT_ASSERT(m_rdmaClient->remoteAlloc(Config::getIPFromNodeId(nodeId), Config::DPI_SEGMENT_SIZE, remoteOffset));
//   std::cout << "Allocated new segment" << '\n';

//   //ACT 5
//   fetchedCounter = buffWriter.modifyCounter(expected, remoteOffset + Config::DPI_SEGMENT_HEADER_META::getCounterOffset);

//   //ASSERT 5
//   rdma_buffer = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(remoteOffset);
//   CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[0].counter);
//   CPPUNIT_ASSERT_EQUAL(fetched, fetchedCounter);
//   CPPUNIT_ASSERT_EQUAL((uint64_t)0, rdma_buffer[0].hasFollowSegment);

//   //ACT 6
//   subtract = -5;
//   fetchedCounter = buffWriter.modifyCounter(subtract, remoteOffset);

//   //ASSERT 6
//   rdma_buffer = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(remoteOffset);
//   CPPUNIT_ASSERT_EQUAL((uint64_t)((int64_t)expected + subtract), rdma_buffer[0].counter);
//   CPPUNIT_ASSERT_EQUAL(expected, fetchedCounter);
//   CPPUNIT_ASSERT_EQUAL((uint64_t)0, rdma_buffer[0].hasFollowSegment);

//   CPPUNIT_ASSERT(buffWriter.close());
// }