#include "TestBufferWriter.h"
#include "../../net/rdma/RDMAClient.h"

std::atomic<int> TestBufferWriter::bar{0};    // Counter of threads, faced barrier.
std::atomic<int> TestBufferWriter::passed{0}; // Number of barriers, passed by all threads.
void TestBufferWriter::setUp()
{
  //Setup Test DPI
  Config::RDMA_MEMSIZE = 1024ul * 1024 * 1024 * 1;  //1GB
  Config::DPI_SEGMENT_SIZE = (2048 + sizeof(Config::DPI_SEGMENT_HEADER_t));
  Config::DPI_INTERNAL_BUFFER_SIZE = 1024;
  Config::DPI_REGISTRY_SERVER = "127.0.0.1";
  Config::DPI_NODES.clear();
  string dpiTestNode = "127.0.0.1:" + to_string(Config::DPI_NODE_PORT);
  Config::DPI_NODES.push_back(dpiTestNode);

  m_nodeClient = new NodeClient();
  m_stub_regClient = new RegistryClientStub();
  m_nodeServer = new NodeServer();
  CPPUNIT_ASSERT(m_nodeServer->startServer());
};

void TestBufferWriter::tearDown()
{
  delete m_nodeClient;
  delete m_stub_regClient;
  if (m_nodeServer->running())
  {
    m_nodeServer->stopServer();
  }
  delete m_nodeServer;
};

void TestBufferWriter::testBuffer()
{
  //ARRANGE
  size_t numberElements = 5000;
  //ACT
  int *rdma_buffer = (int *)m_nodeServer->getBuffer(0);
  //ASSERT
  for (uint32_t i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(rdma_buffer[i] == 0);
  }
};

void TestBufferWriter::testAppend_SimpleData()
{
  //ARRANGE
  string bufferName = "test";

  size_t remoteOffset = 0;
  // size_t memSize = sizeof(int);

  uint32_t numberSegments = 2;
  size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int) * numberSegments;

  BufferHandle *buffHandle = new BufferHandle(bufferName, 1, 1);
  m_stub_regClient->registerBuffer(buffHandle);
  BufferWriter buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);

  //ACT
  for (size_t i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(buffWriter.append((void *)&i, sizeof(int)));
  }

  CPPUNIT_ASSERT(buffWriter.close());
  int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);

  //ASSERT
  for (uint32_t j = 0; j < numberSegments; j++)
  {
    int expected = 0;
    for (uint32_t i = sizeof(Config::DPI_SEGMENT_HEADER_t) / sizeof(int); i < (numberElements / numberSegments); i++)
    {
      CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[i]);
      expected++;
    }
  }
};

void TestBufferWriter::testAppend_SplitData()
{
  string bufferName = "test";

  size_t remoteOffset = 0;

  size_t numberElements = (Config::DPI_INTERNAL_BUFFER_SIZE / sizeof(int)) * 4;
  uint32_t numberSegments = (numberElements * sizeof(int)) / (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t));
  size_t memSize = numberElements * sizeof(int);
  BufferHandle *buffHandle = new BufferHandle(bufferName, 1, 1);
  m_stub_regClient->registerBuffer(buffHandle);
  BufferWriter buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);

  int *data = new int[numberElements];

  for (uint32_t i = 0; i < numberElements; i++)
  {
    data[i] = i;
  }

  CPPUNIT_ASSERT(buffWriter.append((void *)data, memSize));

  CPPUNIT_ASSERT(buffWriter.close());
  int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);

 
    // std::cout << "Buffer " << '\n';
    // for (int i = 0; i < numberElements; i++)
    //     std::cout << rdma_buffer[i] << ' ';

  //ASSERT
  for (uint32_t j = 0; j < numberSegments; j++)
  {
    int expected = 0;
    for (uint32_t i = sizeof(Config::DPI_SEGMENT_HEADER_t) / sizeof(int); i < (numberElements / numberSegments); i++)
    {
      CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[i]);
      expected++;
    }
  }
};

void TestBufferWriter::testAppend_SingleInts()
{
  //ARRANGE
  string bufferName = "test";

  size_t remoteOffset = 0;
  uint32_t numberSegments = 2;
  size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int) * numberSegments;
  uint64_t expectedCounter = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t));
  uint64_t expectedhasFollowSegment = 1;
  BufferHandle *buffHandle = new BufferHandle(bufferName, 1, 1);
  m_stub_regClient->registerBuffer(buffHandle);
  BufferWriter buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);


  //ACT
  for (uint32_t i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(buffWriter.append((void*)&i, sizeof(int)));
  }

  //ASSERT
  CPPUNIT_ASSERT(buffWriter.close());

  Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(remoteOffset);
  int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);

  CPPUNIT_ASSERT_EQUAL(expectedCounter, header[0].counter);
  CPPUNIT_ASSERT_EQUAL(expectedhasFollowSegment, header[0].hasFollowSegment);

  for (uint32_t j = 0; j < numberSegments; j++)
  {
    int expected = 0;
    for (uint32_t i = sizeof(Config::DPI_SEGMENT_HEADER_t) / sizeof(int); i < (numberElements / numberSegments); i++)
    {
      CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[i]);
      expected++;
    }
  }
};



void TestBufferWriter::testAppend_MultipleConcurrentClients()
{
  //ARRANGE

  string bufferName = "test";
  // int nodeId = 1;
  uint64_t expectedHasFollowSegment = 0;
  uint64_t expectedCounter = Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t);
  std::vector<int> expectedResult;
  std::vector<int> result;

  std::vector<TestData> *dataToWrite = new std::vector<TestData>();

  size_t numberElements = expectedCounter / sizeof(TestData);

  //Create data for clients to send
  for (size_t i = 0; i < numberElements; i++)
  {
    dataToWrite->emplace_back(i, i, i, i);
    for (size_t t = 0; t < 4; t++)
    {
      expectedResult.push_back(i);
    }
  }
  BufferHandle *buffHandle = new BufferHandle(bufferName, 1, 1);
  m_stub_regClient->registerBuffer(buffHandle);
  BufferHandle *buffHandle1 = m_stub_regClient->retrieveBuffer(bufferName);
  BufferHandle *buffHandle2 = m_stub_regClient->retrieveBuffer(bufferName);

  BufferWriterClient<TestData> *client1 = new BufferWriterClient<TestData>(m_nodeServer, m_stub_regClient, buffHandle1, dataToWrite);
  BufferWriterClient<TestData> *client2 = new BufferWriterClient<TestData>(m_nodeServer, m_stub_regClient, buffHandle2, dataToWrite);

  //ACT
  client1->start();
  client2->start();
  client1->join();
  client2->join();

  //ASSERT
  int *rdma_buffer = (int *)m_nodeServer->getBuffer(0);

  // std::cout << "Buffer " << '\n';
  // for (int i = 0; i < Config::DPI_SEGMENT_SIZE/sizeof(int)*2; i++)
  // {
  //   std::cout << rdma_buffer[i] << ' ';
  //   // result.push_back(rdma_buffer[i]);
  // }
  //Assert client 1
  for (size_t i = 0; i < buffHandle1->entrySegments.size(); i++)
  {
    Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(buffHandle1->entrySegments[i].offset);
    if (header[0].counter == (uint64_t)0)
      continue;

    CPPUNIT_ASSERT_EQUAL(expectedCounter, header[0].counter);
    CPPUNIT_ASSERT_EQUAL(expectedHasFollowSegment, header[0].hasFollowSegment);

    for (size_t j = (buffHandle1->entrySegments[i].offset + sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int); j < (buffHandle1->entrySegments[i].offset + expectedCounter + sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int); j++)
    {
      result.push_back(rdma_buffer[j]);
    }
  }
  CPPUNIT_ASSERT_EQUAL(expectedResult.size(), result.size());
  for (size_t i = 0; i < expectedResult.size(); i++)
  {
    CPPUNIT_ASSERT_EQUAL(expectedResult[i], result[i]);
  }

  result.clear();

  //Assert client 2
  for (size_t i = 0; i < buffHandle2->entrySegments.size(); i++)
  {
    Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(buffHandle2->entrySegments[i].offset);
    if (header[0].counter == (uint64_t)0)
      continue;

    CPPUNIT_ASSERT_EQUAL(expectedCounter, header[0].counter);
    CPPUNIT_ASSERT_EQUAL(expectedHasFollowSegment, header[0].hasFollowSegment);

    for (size_t j = (buffHandle2->entrySegments[i].offset + sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int); j < (buffHandle2->entrySegments[i].offset + expectedCounter + sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int); j++)
    {
      result.push_back(rdma_buffer[j]);
    }
  }
  CPPUNIT_ASSERT_EQUAL(expectedResult.size(), result.size());
  for (size_t i = 0; i < expectedResult.size(); i++)
  {
    CPPUNIT_ASSERT_EQUAL(expectedResult[i], result[i]);
  }
}



void TestBufferWriter::testAppend_VaryingDataSizes()
{
  //ARRANGE
  string bufferName = "buffer1";
  NodeID nodeId = 1;

  size_t numberElements = 200;
  size_t intsWritten = 0;
  vector<int> expected;

  BufferHandle *buffHandle = new BufferHandle(bufferName, nodeId, 1);
  m_stub_regClient->registerBuffer(buffHandle);
  BufferWriter buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);

  //ACT
  for (size_t i = 0; i < numberElements; i++)
  {
    size_t nrInts = (i%10)+1; //data size ranging from 1 to 10 ints
    int data[nrInts];
    size_t dataSize = sizeof(data);    
    for(size_t j = 0; j < nrInts; j++)
    {
      expected.push_back(nrInts);
      data[j] = nrInts;
    }    
    CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter, (void*)data, dataSize));
    intsWritten += nrInts;
  }

  CPPUNIT_ASSERT(buffWriter.close());
  // int *rdma_buffer = (int *)m_nodeServer->getBuffer(0);

  buffHandle = m_stub_regClient->retrieveBuffer(bufferName);
  // uint32_t numberSegments = buffHandle->entrySegments.size();
  
  // for(size_t i = 0; i < intsWritten + sizeof(Config::DPI_SEGMENT_HEADER_t)/sizeof(int)*numberSegments; i++)
  // {
  //   std::cout << rdma_buffer[i] << " ";
  // }
  
  //ASSERT
  int expectedIdx = 0;
  for (BufferSegment &segment : buffHandle->entrySegments)
  {
    //ASSERT HEADER
    Config::DPI_SEGMENT_HEADER_t *header = readSegmentHeader(&segment);
    if (header[0].hasFollowSegment == (uint64_t)1)
      CPPUNIT_ASSERT_EQUAL(header[0].counter, (uint64_t)(Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)));
    else
      CPPUNIT_ASSERT_EQUAL(header[0].counter, (uint64_t)(intsWritten*sizeof(int) - (buffHandle->entrySegments.size()-1) * (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t))));

    //ASSERT DATA
    size_t size = 0;
    int* segData = (int*)readSegmentData(&segment, size);
    size = size / sizeof(int);
    
    for(size_t i = 0; i < size; i++)
    {
      CPPUNIT_ASSERT_EQUAL(expected[expectedIdx], segData[i]);
      ++expectedIdx;
    }
  }
}



void* TestBufferWriter::readSegmentData(BufferSegment* segment, size_t &size)
{
  void* segmentPtr = m_nodeServer->getBuffer(segment->offset);
  Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *)segmentPtr;
  size = header[0].counter;
  return (void*)(((char*)segmentPtr) + sizeof(Config::DPI_SEGMENT_HEADER_t));
}

Config::DPI_SEGMENT_HEADER_t *TestBufferWriter::readSegmentHeader(BufferSegment* segment)
{
  void* segmentPtr = m_nodeServer->getBuffer(segment->offset);
  Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *)segmentPtr;
  return header;
}


// void TestBufferWriter::testAppendShared_AtomicHeaderManipulation()
// {

//   //ARRANGE
//   string bufferName = "test";

//   uint64_t expected = 10;
//   uint64_t fetched = 0;

//   BufferHandle *buffHandle = m_stub_regClient->createBuffer(bufferName, 1, (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)),Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR);
//   BufferWriter<BufferWriterShared> buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);

//   //ACT
//   auto fetchedCounter = buffWriter.modifyCounter(expected, 0);

//   //ASSERT
//   Config::DPI_SEGMENT_HEADER_t *rdma_buffer = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(0);
//   CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[0].counter);
//   CPPUNIT_ASSERT_EQUAL(fetched, fetchedCounter);
//   CPPUNIT_ASSERT_EQUAL((uint64_t)0, rdma_buffer[0].hasFollowSegment);

//   //ACT 2
//   auto fetchedFollowPage = buffWriter.setHasFollowSegment(0);

//   //ASSERT 2
//   CPPUNIT_ASSERT_EQUAL((uint64_t)1, rdma_buffer[0].hasFollowSegment);
//   CPPUNIT_ASSERT_EQUAL(fetched, fetchedFollowPage);

//   //ACT 3
//   fetchedFollowPage = buffWriter.setHasFollowSegment(0);

//   //ASSERT 3
//   CPPUNIT_ASSERT(buffWriter.close());
//   CPPUNIT_ASSERT_EQUAL((uint64_t)1, rdma_buffer[0].hasFollowSegment);
//   CPPUNIT_ASSERT_EQUAL((uint64_t)1, fetchedFollowPage);
// }

// void TestBufferWriter::testAppendShared_SimpleData()
// {
//   //ARRANGE
//   string bufferName = "buffer1";
//   NodeID nodeId = 1;
//   size_t remoteOffset = 0;
//   size_t memSize = sizeof(int);

//   uint32_t numberSegments = 2;
//   size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / memSize * numberSegments;

//   BufferHandle *buffHandle = m_stub_regClient->createBuffer(bufferName, nodeId, (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)),Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR);
//   BufferWriter<BufferWriterShared> buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);

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
//     CPPUNIT_ASSERT_EQUAL((uint64_t)(Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)), header[0].counter);
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

// void TestBufferWriter::testAppendShared_MultipleConcurrentClients()
// {
//   //ARRANGE
//   string connection = "127.0.0.1:5400";
//   string bufferName = "test";
//   int nodeId = 1;
//   std::vector<int> expectedResult;
//   std::vector<int> result;
//   uint64_t expectedHasFollowSegment = 1;
//   uint64_t expectedCounter = Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t);
//   const size_t expectedSegments = 3; //Writing 2 full segments, but 3 is created since threshold is passed in the second segment

//   std::vector<TestData> *dataToWrite = new std::vector<TestData>();

//   size_t numberElements = expectedCounter / sizeof(TestData);

//   //Create data for clients to send
//   for (size_t i = 0; i < numberElements; i++)
//   {
//     dataToWrite->emplace_back(i, i, i, i);
//     for (size_t t = 0; t < 4 * 2; t++)
//     {
//       expectedResult.push_back(i);
//     }
//   }

//   //Create BufferHandle in advance and remote_alloc the first segment. (Flow needs to be discussed so it is the same between strategies)
//   //Do this because the Shared strategy needs to be passed a BufferHandle that already contains a segment, or else both clients will start by making one each.
//   BufferHandle *buffHandle = new BufferHandle(bufferName, nodeId);
//   RDMAClient *rdmaClient = new RDMAClient();
//   rdmaClient->connect(connection);
//   size_t remoteOffset = 0;
//   rdmaClient->remoteAlloc(connection, Config::DPI_SEGMENT_SIZE, remoteOffset);

//   BufferSegment newSegment(remoteOffset, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t), Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR);
//   buffHandle->entrySegments.push_back(newSegment);

//   m_stub_regClient->registerBuffer(buffHandle);
//   //Need to create a buffHandle for each client, to emulate distributed setting
//   BufferHandle *copy_buffHandle = new BufferHandle(buffHandle->name, buffHandle->node_id);
//   for (auto segment : buffHandle->entrySegments)
//   {
//     copy_buffHandle->entrySegments.push_back(segment);
//   }
//   BufferWriterClient<TestData, BufferWriterShared> *client1 = new BufferWriterClient<TestData, BufferWriterShared>(m_nodeServer, m_stub_regClient, buffHandle, dataToWrite);
//   BufferWriterClient<TestData, BufferWriterShared> *client2 = new BufferWriterClient<TestData, BufferWriterShared>(m_nodeServer, m_stub_regClient, copy_buffHandle, dataToWrite);

//   //ACT
//   client1->start();
//   client2->start();
//   client1->join();
//   client2->join();

//   //ASSERT
//   int *rdma_buffer = (int *)m_nodeServer->getBuffer(0);

//   DebugCode(
//     std::cout << "Buffer " << '\n';
//     for (int i = 0; i < numberElements*2*4; i++)
//         std::cout
//     << rdma_buffer[i] << ' ';);
//   //Assert number of segments
//   CPPUNIT_ASSERT_EQUAL(expectedSegments, m_stub_regClient->retrieveBuffer(bufferName)->entrySegments.size());

//   // std::cout << "Buffer " << '\n';
//   for (size_t i = 0; i < Config::DPI_SEGMENT_SIZE / sizeof(int) * (expectedSegments - 1); i++)
//   {
//     //Assert header
//     if (i % (Config::DPI_SEGMENT_SIZE / sizeof(int)) == 0)
//     {
//       // std::cout << "Asserting header" << '\n';
//       Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(i * sizeof(int));
//       CPPUNIT_ASSERT_EQUAL_MESSAGE("Segment counter did not match", expectedCounter, header[0].counter);
//       CPPUNIT_ASSERT_EQUAL_MESSAGE("Segment has follow segment did not match", expectedHasFollowSegment, header[0].hasFollowSegment);
//       i += sizeof(Config::DPI_SEGMENT_HEADER_t) / sizeof(int) - 1; //Skip asserted header
//     }
//     else
//     {
//       // std::cout << rdma_buffer[i] << ' ';
//       result.push_back(rdma_buffer[i]);
//     }
//   }

//   std::sort(expectedResult.begin(), expectedResult.end());
//   std::sort(result.begin(), result.end());

//   CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected result size did not match result size", expectedResult.size(), result.size());

//   for (size_t i = 0; i < expectedResult.size(); i++)
//   {
//     CPPUNIT_ASSERT_EQUAL_MESSAGE("Integer result did not match", expectedResult[i], result[i]);
//   }
// }


// void TestBufferWriter::testAppendShared_VaryingDataSizes()
// {
//   //ARRANGE
//   string bufferName = "buffer1";
//   NodeID nodeId = 1;

//   size_t numberElements = 200;
//   size_t intsWritten = 0;
//   vector<int> expected;

//   BufferHandle *buffHandle = m_stub_regClient->createBuffer(bufferName, nodeId, (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)), Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR);
//   BufferWriter<BufferWriterShared> buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);

//   //ACT
//   for (size_t i = 0; i < numberElements; i++)
//   {
//     size_t nrInts = (i%10)+1; //data size ranging from 1 to 10 ints
//     int data[nrInts];
//     size_t dataSize = sizeof(data);    
//     for(size_t j = 0; j < nrInts; j++)
//     {
//       expected.push_back(nrInts);
//       data[j] = nrInts;
//     }    
//     CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter, (void*)data, dataSize));
//     intsWritten += nrInts;
//   }

//   CPPUNIT_ASSERT(buffWriter.close());
//   // int *rdma_buffer = (int *)m_nodeServer->getBuffer(0);

//   buffHandle = m_stub_regClient->retrieveBuffer(bufferName);
//   // uint32_t numberSegments = buffHandle->entrySegments.size();

//   // for(size_t i = 0; i < intsWritten + sizeof(Config::DPI_SEGMENT_HEADER_t)/sizeof(int)*numberSegments; i++)
//   // {
//   //   std::cout << rdma_buffer[i] << " ";
//   // }
  
//   //ASSERT
//   int expectedIdx = 0;
//   for (BufferSegment &segment : buffHandle->entrySegments)
//   {
//     //ASSERT HEADER
//     Config::DPI_SEGMENT_HEADER_t *header = readSegmentHeader(&segment);
//     if (header[0].hasFollowSegment == (uint64_t)1)
//       CPPUNIT_ASSERT_EQUAL(header[0].counter, (uint64_t)(Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)));
//     else
//       CPPUNIT_ASSERT_EQUAL(header[0].counter, (uint64_t)(intsWritten*sizeof(int) - (buffHandle->entrySegments.size()-1) * (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t))));

//     //ASSERT DATA
//     size_t size = 0;
//     int* segData = (int*)readSegmentData(&segment, size);
//     size = size / sizeof(int);
    
//     for(size_t i = 0; i < size; i++)
//     {
//       CPPUNIT_ASSERT_EQUAL_MESSAGE("Data check", expected[expectedIdx], segData[i]);
//       ++expectedIdx;
//     }
//   }
// }
