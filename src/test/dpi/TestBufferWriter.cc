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
  Config::DPI_REGISTRY_PORT = 5300;
  Config::DPI_NODE_PORT = 5400;
  Config::DPI_NODES.clear();
  string dpiTestNode = "127.0.0.1:" + to_string(Config::DPI_NODE_PORT);
  Config::DPI_NODES.push_back(dpiTestNode);

  m_nodeServer = new NodeServer();
  CPPUNIT_ASSERT(m_nodeServer->startServer());

  m_regServer = new RegistryServer();
  std::cout << "Start RegServer" << '\n';
  CPPUNIT_ASSERT(m_regServer->startServer());
  std::cout << "Started RegServer" << '\n';

  m_regClient = new RegistryClient();
};

void TestBufferWriter::tearDown()
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
  std::cout << "Test" << '\n';
  m_regClient->registerBuffer(new BufferHandle(bufferName, 1, numberSegments, 1, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)));
  std::cout << "Reg" << '\n';
  // auto buffHandle = m_regClient->joinBuffer(bufferName);
  // CPPUNIT_ASSERT(buffHandle != nullptr);
  // CPPUNIT_ASSERT_EQUAL(buffHandle->name, bufferName);
  // CPPUNIT_ASSERT_EQUAL(1, ((int)buffHandle->entrySegments.size()));
  // CPPUNIT_ASSERT_EQUAL(1, ((int)buffHandle->node_id));
  // std::cout << "Join" << '\n';

  BufferWriterBW buffWriter(bufferName,m_regClient, Config::DPI_INTERNAL_BUFFER_SIZE);

  std::cout << "BufferWriterBW" << '\n';

  //ACT
  for (size_t i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(buffWriter.append((void *)&i, sizeof(int)));
  }

  std::cout << "Appending" << '\n';
  CPPUNIT_ASSERT(buffWriter.close());
  int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);

  std::cout << "Assert" << '\n';
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

  std::cout << "Finished " << '\n';
};

void TestBufferWriter::testAppend_SplitData()
{
  string bufferName = "test";

  size_t remoteOffset = 0;

  size_t numberElements = (Config::DPI_INTERNAL_BUFFER_SIZE / sizeof(int)) * 4;
  uint32_t numberSegments = (numberElements * sizeof(int)) / (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t));
  size_t memSize = numberElements * sizeof(int);
  m_regClient->registerBuffer(new BufferHandle(bufferName, 1, numberSegments, 1, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)));
  // auto buffHandle = m_regClient->joinBuffer(bufferName);
  BufferWriterBW buffWriter(bufferName,m_regClient, Config::DPI_INTERNAL_BUFFER_SIZE);

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
  m_regClient->registerBuffer(new BufferHandle(bufferName, 1, numberSegments, 1, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)));
  // auto buffHandle = m_regClient->joinBuffer(bufferName);
  BufferWriterBW buffWriter(bufferName,m_regClient, Config::DPI_INTERNAL_BUFFER_SIZE);

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
  CPPUNIT_ASSERT(!header[0].isEndSegment());

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
  m_regClient->registerBuffer(new BufferHandle(bufferName, 1, 2, 2, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)));
  auto buffHandle = m_regClient->retrieveBuffer(bufferName);
  

  BufferWriterClient<TestData> *client1 = new BufferWriterClient<TestData>(m_nodeServer, bufferName, dataToWrite);
  BufferWriterClient<TestData> *client2 = new BufferWriterClient<TestData>(m_nodeServer, bufferName, dataToWrite);

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

  //Assert Both Clients

  std::cout << "buffHandle->entrySegments.size() " << buffHandle->entrySegments.size()<< '\n';
  for (size_t i = 0; i < buffHandle->entrySegments.size(); i++)
  {
    Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(buffHandle->entrySegments[i].offset);
    if (header[0].counter == (uint64_t)0)
      continue;

    CPPUNIT_ASSERT_EQUAL(expectedCounter, header[0].counter);

    for (size_t j = (buffHandle->entrySegments[i].offset + sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int); j < (buffHandle->entrySegments[i].offset + expectedCounter + sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int); j++)
    {
      result.push_back(rdma_buffer[j]);
    }
  }
  CPPUNIT_ASSERT_EQUAL(expectedResult.size() * 2, result.size());
  for (size_t i = 0; i < expectedResult.size() * 2; i++)
  {
    CPPUNIT_ASSERT_EQUAL(expectedResult[i % expectedResult.size() ], result[i]);
  }

  result.clear();

  // //Assert client 2
  // for (size_t i = 0; i < buffHandle->entrySegments.size(); i++)
  // {
  //   Config::DPI_SEGMENT_HEADER_t *header = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(buffHandle->entrySegments[i].offset);
  //   if (header[0].counter == (uint64_t)0)
  //     continue;

  //   CPPUNIT_ASSERT_EQUAL(expectedCounter, header[0].counter);

  //   for (size_t j = (buffHandle->entrySegments[i].offset + sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int); j < (buffHandle->entrySegments[i].offset + expectedCounter + sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int); j++)
  //   {
  //     result.push_back(rdma_buffer[j]);
  //   }
  // }
  // CPPUNIT_ASSERT_EQUAL(expectedResult.size(), result.size());
  // for (size_t i = 0; i < expectedResult.size(); i++)
  // {
  //   CPPUNIT_ASSERT_EQUAL(expectedResult[i], result[i]);
  // }
}

void TestBufferWriter::testAppend_VaryingDataSizes()
{
  //ARRANGE
  string bufferName = "buffer1";
  NodeID nodeId = 1;

  size_t numberElements = 200;
  size_t intsWritten = 0;
  vector<int> expected;

  m_regClient->registerBuffer(new BufferHandle(bufferName, nodeId, 5, 1, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)));
  auto buffHandle = m_regClient->retrieveBuffer(bufferName);
  size_t offset = buffHandle->entrySegments.at(0).offset;
  BufferWriterBW buffWriter(bufferName,m_regClient, Config::DPI_INTERNAL_BUFFER_SIZE);

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
  while (!header->isEndSegment())
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

  size_t segmentsUsed = intsWritten * sizeof(int) / (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) + 1;

  CPPUNIT_ASSERT_EQUAL_MESSAGE("Counter in last segment did not match expected", (uint64_t)(intsWritten * sizeof(int) - (segmentsUsed - 1) * (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t))), header->counter);
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
