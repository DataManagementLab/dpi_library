#include "TestBufferWriter.h"
#include "../../net/rdma/RDMAClient.h"

std::atomic<int> TestBufferWriter::bar{0};    // Counter of threads, faced barrier.
std::atomic<int> TestBufferWriter::passed{0}; // Number of barriers, passed by all threads.
void TestBufferWriter::setUp()
{
  //Setup Test DPI
  Config::RDMA_MEMSIZE = 1024ul * 1024 * 1024 * 1; //1GB
  Config::DPI_SEGMENT_SIZE = (2048 + sizeof(Config::DPI_SEGMENT_HEADER_t));
  Config::DPI_INTERNAL_BUFFER_SIZE = 1024;
  Config::DPI_REGISTRY_SERVER = "127.0.0.1";
  Config::DPI_NODES.clear();
  string dpiTestNode = "127.0.0.1:" + to_string(Config::DPI_NODE_PORT);
  Config::DPI_NODES.push_back(dpiTestNode);

  m_rdmaClient = new RDMAClient();
  m_stub_regClient = new RegistryClientStub(m_rdmaClient);
  m_nodeServer = new NodeServer();
  CPPUNIT_ASSERT(m_nodeServer->startServer());
};

void TestBufferWriter::tearDown()
{
  delete m_rdmaClient;
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

  m_stub_regClient->registerBuffer(new BufferHandle(bufferName, 1, numberSegments));
  auto buffHandle = m_stub_regClient->createSegmentRingOnBuffer(bufferName);
  BufferWriterBW buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);

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
  m_stub_regClient->registerBuffer(new BufferHandle(bufferName, 1, numberSegments));
  auto buffHandle = m_stub_regClient->createSegmentRingOnBuffer(bufferName);
  BufferWriterBW buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);

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
  m_stub_regClient->registerBuffer(new BufferHandle(bufferName, 1, numberSegments));
  auto buffHandle = m_stub_regClient->createSegmentRingOnBuffer(bufferName);
  BufferWriterBW buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);

  //ACT
  for (uint32_t i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(buffWriter.append((void *)&i, sizeof(int)));
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
  m_stub_regClient->registerBuffer(new BufferHandle(bufferName, 1, 2));
  auto buffHandle1 = m_stub_regClient->createSegmentRingOnBuffer(bufferName);
  auto buffHandle2 = m_stub_regClient->createSegmentRingOnBuffer(bufferName);

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

  m_stub_regClient->registerBuffer(new BufferHandle(bufferName, nodeId, 5));
  auto buffHandle = m_stub_regClient->createSegmentRingOnBuffer(bufferName);
  size_t offset = buffHandle->entrySegments.at(0).offset;
  BufferWriterBW buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);

  //ACT
  for (size_t i = 0; i < numberElements; i++)
  {
    size_t nrInts = (i % 10) + 1; //data size ranging from 1 to 10 ints
    int data[nrInts];
    size_t dataSize = sizeof(data);
    for (size_t j = 0; j < nrInts; j++)
    {
      expected.push_back(nrInts);
      data[j] = nrInts;
    }
    CPPUNIT_ASSERT(buffWriter.append((void *)data, dataSize));
    intsWritten += nrInts;
  }

  CPPUNIT_ASSERT(buffWriter.close());
  // int *rdma_buffer = (int *)m_nodeServer->getBuffer(0);

  // std::cout << "\n";
  // for (size_t i = 0; i < intsWritten + sizeof(Config::DPI_SEGMENT_HEADER_t) * 3; i++)
  // {
  //   if (i % (Config::DPI_SEGMENT_SIZE / sizeof(int)) == 0)
  //     std::cout << "\n";
  //   std::cout << rdma_buffer[i] << " ";
  // }
  // std::cout << "\n";

  //ASSERT
  int expectedIdx = 0;
  Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *)((char *)m_nodeServer->getBuffer() + offset);
  while (header->hasFollowSegment == (uint64_t)1)
  {
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Counter in not-last segment did not match expected, segment offset: " + to_string(offset), (uint64_t)(Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)), header->counter);

    //ASSERT DATA
    size_t size = 0;
    int *segData = (int *)readSegmentData(offset, size);
    size = size / sizeof(int);

    for (size_t i = 0; i < size; i++)
    {
      CPPUNIT_ASSERT_EQUAL(expected[expectedIdx], segData[i]);
      ++expectedIdx;
    }

    offset = header->nextSegmentOffset;
    header = readSegmentHeader(offset);
  }

  size_t segmentsUsed = intsWritten*sizeof(int)/(Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) + 1;

  CPPUNIT_ASSERT_EQUAL_MESSAGE("Counter in last segment did not match expected", (uint64_t)(intsWritten * sizeof(int) - (segmentsUsed-1) * (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t))), header->counter);
  //ASSERT DATA
  size_t size = 0;
  int *segData = (int *)readSegmentData(offset, size);
  size = size / sizeof(int);

  for (size_t i = 0; i < size; i++)
  {
    CPPUNIT_ASSERT_EQUAL(expected[expectedIdx], segData[i]);
    ++expectedIdx;
  }

}

void *TestBufferWriter::readSegmentData(BufferSegment *segment, size_t &size)
{
  return readSegmentData(segment->offset, size);
}

void *TestBufferWriter::readSegmentData(size_t offset, size_t &size)
{
  void *segmentPtr = m_nodeServer->getBuffer(offset);
  Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *)segmentPtr;
  size = header->counter;
  return ++header;
}

Config::DPI_SEGMENT_HEADER_t *TestBufferWriter::readSegmentHeader(BufferSegment *segment)
{
  return readSegmentHeader(segment->offset);
}

Config::DPI_SEGMENT_HEADER_t *TestBufferWriter::readSegmentHeader(size_t offset)
{
  void *segmentPtr = m_nodeServer->getBuffer(offset);
  Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *)segmentPtr;
  return header;
}
