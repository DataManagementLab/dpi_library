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

void IntegrationTestsAppend::SimpleIntegrationWithAppendInts_BW()
{
  //ARRANGE
  string bufferName = "buffer1";
    
  size_t remoteOffset = 0;
  size_t memSize = sizeof(int);

  uint32_t numberSegments = 2;
  size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / memSize * numberSegments;
  // uint32_t segmentsPerWriter = 2;
  BufferHandle *buffHandle = new BufferHandle(bufferName, 1,  3, 1, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)); //Create 1 less segment in ring to test BufferWriterBW creating a segment on the ring
  DPI_DEBUG("Created BufferHandle\n");
  m_regClient->registerBuffer(buffHandle);
  DPI_DEBUG("Registered Buffer in Registry\n");
  // buffHandle = m_regClient->joinBuffer(bufferName);
  // DPI_DEBUG("Created segment ring on buffer\n");


  int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);
  // for(size_t i = 0; i < numberSegments*Config::DPI_SEGMENT_SIZE/memSize; i++)
  // {
  //   std::cout << rdma_buffer[i] << " ";
  // }

  BufferWriterBW buffWriter(bufferName, m_regClient, Config::DPI_INTERNAL_BUFFER_SIZE);

  //ACT
  for (size_t i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter, (void *)&i, memSize));
  }

  CPPUNIT_ASSERT(buffWriter.close());
  
  // for(size_t i = 0; i < numberSegments*Config::DPI_SEGMENT_SIZE/memSize; i++)
  // {
  //   std::cout << rdma_buffer[i] << " ";
  // }
  
  //ASSERT
  for (uint32_t j = 0; j < numberSegments; j++)
  {
    //Assert header
    Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *) &(rdma_buffer[j*Config::DPI_SEGMENT_SIZE / memSize]);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected size of segment (without header) did not match segment counter", (uint64_t)(Config::DPI_SEGMENT_SIZE- sizeof(Config::DPI_SEGMENT_HEADER_t)), header[0].counter);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("isEndSegment did not match expected", (j == numberSegments - 1 ? true : false), header[0].isEndSegment());

    //Assert data
    int expected = 0;
    for (uint32_t i = sizeof(Config::DPI_SEGMENT_HEADER_t) / memSize; i < (numberElements / numberSegments); i++)
    {
      CPPUNIT_ASSERT_EQUAL_MESSAGE("Data value did not match", expected, rdma_buffer[i]);
      expected++;
    }
  }
}


void IntegrationTestsAppend::FourAppendersConcurrent_BW()
{
  //ARRANGE
  string bufferName = "test";
  int nodeId = 1;

  int64_t numClients = 4;
  size_t segPerClient = 2;
  int64_t numberElements = segPerClient * (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int64_t); //Each client fill two segments
  std::vector<int64_t> *dataToWrite = new std::vector<int64_t>();
  
  for(int64_t i = 0; i < numberElements; i++)
  {
    dataToWrite->push_back(i);
  }  

  CPPUNIT_ASSERT(m_regClient->registerBuffer(new BufferHandle(bufferName, nodeId, segPerClient, 4, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t))));
  // BufferHandle *buffHandle1 = m_regClient->joinBuffer(bufferName);
  // BufferHandle *buffHandle2 = m_regClient->joinBuffer(bufferName);
  // BufferHandle *buffHandle3 = m_regClient->joinBuffer(bufferName);
  // BufferHandle *buffHandle4 = m_regClient->joinBuffer(bufferName);

  int64_t *rdma_buffer = (int64_t *)m_nodeServer->getBuffer();

  // std::cout << "Buffer before appending" << '\n';
  // for (int i = 0; i < Config::DPI_SEGMENT_SIZE/sizeof(int64_t)*segPerClient*4; i++)
  // {
  //   std::cout << rdma_buffer[i] << ' ';
  // }

  BufferWriterClient<int64_t> *client1 = new BufferWriterClient<int64_t>(bufferName, dataToWrite);
  BufferWriterClient<int64_t> *client2 = new BufferWriterClient<int64_t>(bufferName, dataToWrite);
  BufferWriterClient<int64_t> *client3 = new BufferWriterClient<int64_t>(bufferName, dataToWrite);
  BufferWriterClient<int64_t> *client4 = new BufferWriterClient<int64_t>(bufferName, dataToWrite);

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


  auto handle_ret = m_regClient->retrieveBuffer(bufferName);

  int64_t count = 0;
  auto bufferIterator = handle_ret->getIterator((char *)m_nodeServer->getBuffer());

  size_t iterCounter = 0;

  while (bufferIterator.has_next())
  {
    size_t dataSize;
    int64_t *data = (int64_t *)bufferIterator.next(dataSize);
    int64_t start_counter = (iterCounter / numClients) * (numberElements / segPerClient);
    for (int64_t i = start_counter; i < (int64_t)((dataSize   / sizeof(int64_t)) + start_counter); i++, data++)
    {
      CPPUNIT_ASSERT_EQUAL(i, *data);
      count++;
    }
    iterCounter++;
  }

  CPPUNIT_ASSERT_EQUAL(numberElements * numClients, count);
  m_nodeServer->localFree(rdma_buffer);
}



void IntegrationTestsAppend::SimpleAppendAndConsume_LAT()
{

  //ARRANGE
  string bufferName = "buffer1";

  // size_t remoteOffset = 0;
  size_t memSize = sizeof(int);

  size_t numberSegments = 200;
  size_t numberElements = numberSegments;
  BufferHandle *buffHandle = new BufferHandle(bufferName, 1, numberSegments, 1, memSize, BufferHandle::Buffertype::LAT);
  DPI_DEBUG("Created BufferHandle\n");
  m_regClient->registerBuffer(buffHandle);
  DPI_DEBUG("Registered Buffer in Registry\n");

  BufferWriterLat buffWriter(bufferName, m_regClient, Config::DPI_INTERNAL_BUFFER_SIZE);

  for (size_t i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(buffWriter.append((void *)&i, memSize));
  }

  std::cout << "Finished appending. Closing..." << '\n';

  CPPUNIT_ASSERT(buffWriter.close());


  // std::cout << "Buffer:" << '\n';
  // auto bufPtr = (int *)m_nodeServer->getBuffer();

  // for (size_t i = 0; i < numberElements + (numberSegments * sizeof(Config::DPI_SEGMENT_HEADER_t) / memSize); i++)
  // {
  //   std::cout << bufPtr[i] << " ";
  // }

  auto handle_ret = m_regClient->retrieveBuffer(bufferName);
  std::cout << "Interator creation" << '\n';

  int count = 0;
  BufferIteratorLat bufferIterator = handle_ret->getIteratorLat((char *)m_nodeServer->getBuffer());

  std::cout << "Interator enters while" << '\n';
  while (bufferIterator.has_next())
  {
    // std::cout << "Interator Segment" << '\n';
    size_t dataSize;
    int *data = (int *)bufferIterator.next(dataSize);
    for (size_t i = 0; i < dataSize / memSize; i++, data++)
    {
      // std::cout << "data " << *data << " size: " << memSize << '\n';
      CPPUNIT_ASSERT_EQUAL(count, *data);
      count++;
    }
  }
  CPPUNIT_ASSERT_EQUAL((int)numberElements, count);
}


void IntegrationTestsAppend::FourAppendersConcurrent_LAT()
{
  //ARRANGE
  string bufferName = "test";
  int nodeId = 1;

  auto numClients = 4;
  size_t segsPerRing = 50;  //# of segments in each ring
  size_t segPerClient = 200; //# of segments each client will write
  int64_t numberElements = segPerClient;
  size_t memSize = sizeof(int64_t);
  std::vector<int64_t> *dataToWrite = new std::vector<int64_t>();
  vector<size_t> countData(numberElements);

  for (int64_t i = 0; i < numberElements; i++)
  {
    dataToWrite->push_back(i);
  }

  CPPUNIT_ASSERT(m_regClient->registerBuffer(new BufferHandle(bufferName, nodeId, segsPerRing, numClients, memSize, BufferHandle::Buffertype::LAT)));

  BufferWriterClient<int64_t> *client1 = new BufferWriterClient<int64_t>(bufferName, dataToWrite, numClients, BufferHandle::Buffertype::LAT);
  BufferWriterClient<int64_t> *client2 = new BufferWriterClient<int64_t>(bufferName, dataToWrite, numClients, BufferHandle::Buffertype::LAT);
  BufferWriterClient<int64_t> *client3 = new BufferWriterClient<int64_t>(bufferName, dataToWrite, numClients, BufferHandle::Buffertype::LAT);
  BufferWriterClient<int64_t> *client4 = new BufferWriterClient<int64_t>(bufferName, dataToWrite, numClients, BufferHandle::Buffertype::LAT);

  //ACT & ASSERT
  auto nodeServer = m_nodeServer;
  auto regClient = m_regClient;
  size_t segmentsConsumed = 0;
  std::thread consumer([nodeServer, &bufferName, numClients, &segmentsConsumed, numberElements, segPerClient, regClient, &countData]() {
    auto handle_ret = regClient->retrieveBuffer(bufferName);

    int64_t count = 0;
    auto bufferIterator = handle_ret->getIteratorLat((char *)nodeServer->getBuffer());

    size_t iterCounter = 0;

    while (bufferIterator.has_next())
    {
      // std::cout << "Interator Segment" << '\n';
      size_t dataSize;
      int64_t *data = (int64_t *)bufferIterator.next(dataSize);
      int64_t start_counter = (int64_t)(iterCounter / numClients) * (numberElements / segPerClient);
      for (int64_t i = start_counter; i < (int64_t)((dataSize   / sizeof(int64_t)) + start_counter); i++, data++)
      {
        countData[i]++;        
        count++;
      }
      iterCounter++;
      segmentsConsumed++;
    }

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Number of written elements did not match expected", numberElements * numClients, count); 
  });

  client1->start();
  client2->start();
  client3->start();
  client4->start();

  client1->join();
  client2->join();
  client3->join();
  client4->join();
  consumer.join();

  //Assert each value was read numClients times by consumer
  for (int64_t i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT_EQUAL((size_t)numClients, countData[i]);
  }

  CPPUNIT_ASSERT_EQUAL_MESSAGE("Consumed number of segments did not match expected", (size_t)segPerClient * numClients, (size_t)segmentsConsumed);

}
