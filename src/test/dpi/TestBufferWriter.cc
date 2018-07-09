#include "TestBufferWriter.h"
#include "../../net/rdma/RDMAClient.h"

void TestBufferWriter::setUp()
{

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

void TestBufferWriter::testAppendPrivate_WithoutScratchpad()
{
  //ARRANGE
  string bufferName = "test";
  string connection = "127.0.0.1:5400";
  size_t remoteOffset = 0;
  size_t memSize = sizeof(int);

  uint32_t numberSegments = 2;
  size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int) * numberSegments;

  BuffHandle *buffHandle = m_stub_regClient->dpi_create_buffer(bufferName, 1, connection);
  BufferWriter<BufferWriterPrivate> buffWriter(buffHandle, Config::DPI_SCRATCH_PAD_SIZE, m_stub_regClient);

  //ACT
  for (uint32_t i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(buffWriter.append((void *)&i, memSize));
  }

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

void TestBufferWriter::testAppendPrivate_WithoutScratchpad_splitData()
{
  string bufferName = "test";
  string connection = "127.0.0.1:5400";
  size_t remoteOffset = 0;

  size_t numberElements = (Config::DPI_SCRATCH_PAD_SIZE / sizeof(int)) * 4;
  uint32_t numberSegments = (numberElements * sizeof(int)) / (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t));
  size_t memSize = numberElements * sizeof(int);
  BuffHandle *buffHandle = m_stub_regClient->dpi_create_buffer(bufferName, 1, connection);
  BufferWriter<BufferWriterPrivate> buffWriter(buffHandle, Config::DPI_SCRATCH_PAD_SIZE, m_stub_regClient);

  int *data = new int[numberElements];

  for (uint32_t i = 0; i < numberElements; i++)
  {
    data[i] = i;
  }

  CPPUNIT_ASSERT(buffWriter.append((void *)data, memSize));

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

void TestBufferWriter::testAppendPrivate_WithScratchpad()
{
  //ARRANGE
  string bufferName = "test";
  string connection = "127.0.0.1:5400";
  size_t remoteOffset = 0;
  uint32_t numberSegments = 2;
  size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int) * numberSegments;

  BuffHandle *buffHandle = m_stub_regClient->dpi_create_buffer(bufferName, 1, connection);
  BufferWriter<BufferWriterPrivate> buffWriter(buffHandle, Config::DPI_SCRATCH_PAD_SIZE, m_stub_regClient);

  int *scratchPad = (int *)buffWriter.getScratchPad();

  //ACT
  //Fill ScratchPad and append when it is full or last iteration
  int scratchIter = 0;
  for (uint32_t i = 0; i <= numberElements; i++)
  {
    if ((i % (Config::DPI_SCRATCH_PAD_SIZE / sizeof(int))) == 0 && i > 0)
    {
      std::cout << "appending at " << i << std::endl;
      CPPUNIT_ASSERT(buffWriter.appendFromScratchpad(Config::DPI_SCRATCH_PAD_SIZE));
      scratchIter = 0;
    }
    scratchPad[scratchIter] = i;
    scratchIter++;
  }

  int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);

  for (uint32_t j = 0; j < numberSegments; j++)
  {
    int expected = 0;
    for (uint32_t i = sizeof(Config::DPI_SEGMENT_HEADER_t) / sizeof(int); i < (numberElements / numberSegments); i++)
    {
      CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[i]);
      expected++;
    }
  }

  // CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter,(void*) &data, sizeof(int)));
};

void TestBufferWriter::testAppendPrivate_MultipleClients_WithScratchpad()
{
  //ARRANGE
  string bufferName = "test";
  string connection = "127.0.0.1:5400";

  //Client 1
  uint32_t numberSegments1 = 2;
  size_t numberElements1 = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int) * numberSegments1;

  BuffHandle *buffHandle = m_stub_regClient->dpi_create_buffer(bufferName, 1, connection);
  BufferWriter<BufferWriterPrivate> buffWriter1(buffHandle, Config::DPI_SCRATCH_PAD_SIZE, m_stub_regClient);

  int *scratchPad1 = (int *)buffWriter1.getScratchPad();

  //Client 2
  uint32_t numberSegments2 = 2;
  size_t numberElements2 = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int) * numberSegments2;

  BuffHandle *buffHandle2 = m_stub_regClient->dpi_create_buffer(bufferName, 1, connection);
  BufferWriter<BufferWriterPrivate> buffWriter2(buffHandle2, Config::DPI_SCRATCH_PAD_SIZE, m_stub_regClient);

  int *scratchPad2 = (int *)buffWriter2.getScratchPad();

  //ACT
  //Client 1 append
  //Fill ScratchPad and append when it is full
  int scratchIter1 = 0;
  for (uint32_t i = 0; i <= numberElements1; i++)
  {
    if ((i % (Config::DPI_SCRATCH_PAD_SIZE / sizeof(int))) == 0 && i > 0)
    {
      std::cout << "appending at " << i << std::endl;
      CPPUNIT_ASSERT(buffWriter1.appendFromScratchpad(Config::DPI_SCRATCH_PAD_SIZE));
      scratchIter1 = 0;
    }
    scratchPad1[scratchIter1] = i;
    scratchIter1++;
  }

  //Client 2 append
  int scratchIter2 = 0;
  for (uint32_t i = 0; i <= numberElements2; i++)
  {
    if ((i % (Config::DPI_SCRATCH_PAD_SIZE / sizeof(int))) == 0 && i > 0)
    {
      std::cout << "appending at " << i << std::endl;
      CPPUNIT_ASSERT(buffWriter2.appendFromScratchpad(Config::DPI_SCRATCH_PAD_SIZE));
      scratchIter2 = 0;
    }
    scratchPad2[scratchIter2] = i;
    scratchIter2++;
  }

  //ASSERT
  int *rdma_buffer = (int *)m_nodeServer->getBuffer(0);

  //Client 1
  for (uint32_t j = 0; j < numberSegments1; j++)
  {
    int expected = 0;
    for (uint32_t i = sizeof(Config::DPI_SEGMENT_HEADER_t) / sizeof(int); i < (numberElements1 / numberSegments1); i++)
    {
      CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[i]);
      expected++;
    }
  }

  //Client 2
  for (uint32_t j = 0; j < numberSegments2; j++)
  {
    int expected = 0;
    for (uint32_t i = numberSegments1 * Config::DPI_SEGMENT_SIZE + sizeof(Config::DPI_SEGMENT_HEADER_t) / sizeof(int); i < (numberElements2 / numberSegments2); i++)
    {
      CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[i]);
      expected++;
    }
  }
};

void TestBufferWriter::testAppendPrivate_SizeTooBigForScratchpad()
{
  //ARRANGE
  string bufferName = "test";
  string connection = "127.0.0.1:5400";

  BuffHandle *buffHandle = m_stub_regClient->dpi_create_buffer(bufferName, 1, connection);
  BufferWriter<BufferWriterPrivate> buffWriter(buffHandle, Config::DPI_SCRATCH_PAD_SIZE, m_stub_regClient);

  //ACT
  //ASSERT
  CPPUNIT_ASSERT_MESSAGE("appendFromScratchpad should return false when size is bigger than scratchpad",
                         !buffWriter.appendFromScratchpad(Config::DPI_SCRATCH_PAD_SIZE + 1));
}

void TestBufferWriter::testAppendShared_AtomicHeaderManipulation()
{

  //ARRANGE
  string bufferName = "test";
  string connection = "127.0.0.1:5400";
  uint64_t expected = 10;
  int64_t fetched = 0;

  BuffHandle *buffHandle = m_stub_regClient->dpi_create_buffer(bufferName, 1, connection);
  BufferWriter<BufferWriterShared> buffWriter(buffHandle, Config::DPI_SCRATCH_PAD_SIZE, m_stub_regClient);

  //ACT
  auto fetchedCounter = buffWriter.modifyCounter(expected, 0);

  //ASSERT
  Config::DPI_SEGMENT_HEADER_t *rdma_buffer = (Config::DPI_SEGMENT_HEADER_t *)m_nodeServer->getBuffer(0);
  CPPUNIT_ASSERT_EQUAL(expected, rdma_buffer[0].counter);
  CPPUNIT_ASSERT_EQUAL(fetched, fetchedCounter);
  CPPUNIT_ASSERT_EQUAL((uint64_t)0, rdma_buffer[0].hasFollowPage);

  //ACT 2
  auto fetchedFollowPage = buffWriter.setHasFollowSegment(0 + sizeof(Config::DPI_SEGMENT_HEADER_t::counter));

  //ASSERT 2
  CPPUNIT_ASSERT_EQUAL((uint64_t)1, rdma_buffer[0].hasFollowPage);
  CPPUNIT_ASSERT_EQUAL(fetched, fetchedFollowPage);

    //ACT 3
  fetchedFollowPage = buffWriter.setHasFollowSegment(0 + sizeof(Config::DPI_SEGMENT_HEADER_t::counter));

  //ASSERT 3
  CPPUNIT_ASSERT_EQUAL((uint64_t)1, rdma_buffer[0].hasFollowPage);
  CPPUNIT_ASSERT_EQUAL((int64_t)1, fetchedFollowPage);

}

void TestBufferWriter::testAppendShared_WithScratchpad()
{
  //ARRANGE
  string bufferName = "test";
  string connection = "127.0.0.1:5400";
  size_t remoteOffset = 0;
  uint32_t numberSegments = 4;
  size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int) * numberSegments;

  // Registry buff handle
  BuffHandle *buffHandle = m_stub_regClient->dpi_create_buffer(bufferName, 1, connection);
  BuffHandle *buffHandlePrivateToWriter = new BuffHandle(bufferName, 1, connection);

  BufferWriter<BufferWriterShared> buffWriter(buffHandlePrivateToWriter, Config::DPI_SCRATCH_PAD_SIZE, m_stub_regClient);

  int *scratchPad = (int *)buffWriter.getScratchPad();

  //ACT
  for (uint32_t i = 0; i <= numberElements; i++)
  {
    scratchPad[0] = i;
    CPPUNIT_ASSERT(buffWriter.appendFromScratchpad(sizeof(int)));
  }

  int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);

  DebugCode(
      std::cout << "Buffer " << '\n';
      for (int i = 0; i < numberElements; i++)
          std::cout
      << rdma_buffer[i] << ' ';);

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
}

void TestBufferWriter::testAppendShared_MultipleConcurrentClients()
{
  //ARRANGE
  string connection = "127.0.0.1:5400";
  string bufferName = "test";
  int nodeId = 1;
  std::vector<int> expectedResult;
  std::vector<int> result;

  std::vector<TestData> *dataToWrite = new std::vector<TestData>();
  
  int numberElements = (Config::DPI_SEGMENT_SIZE/4 - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int);

  //Create data for clients to send
  for(size_t i = 0; i < numberElements; i++)
  {
    //number of elements in each send for each client
    dataToWrite->emplace_back(i,i,i,i);
  }
  
  //Create BuffHandle in advance and remote_alloc the first segment. (Flow needs to be discussed so it is the same between strategies)
  //Do this because the Shared strategy needs to be passed a BuffHandle that already contains a segment, or else both clients will start by making one each.
  BuffHandle *buffHandle = new BuffHandle(bufferName, nodeId, connection);
  RDMAClient* rdmaClient = new RDMAClient();
  rdmaClient->connect(connection);
  size_t remoteOffset = 0;
  rdmaClient->remoteAlloc(connection, Config::DPI_SEGMENT_SIZE, remoteOffset);
  BuffSegment newSegment(remoteOffset, Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t), Config::DPI_SEGMENT_SIZE * 0.8);
  buffHandle->segments.push_back(newSegment);
  
  m_stub_regClient->dpi_register_buffer(buffHandle);
  //Need to create a buffHandle for each client, to emulate distributed setting    
  BuffHandle* copy_buffHandle = new BuffHandle(buffHandle->name, buffHandle->node_id, buffHandle->connection);
  for(auto segment : buffHandle->segments){
    copy_buffHandle->segments.push_back(segment); 
  }
  BufferWriterSharedClient* client1 = new BufferWriterSharedClient(m_nodeServer, m_stub_regClient, buffHandle, dataToWrite);
  BufferWriterSharedClient* client2 = new BufferWriterSharedClient(m_nodeServer, m_stub_regClient, copy_buffHandle, dataToWrite);

  //ACT
  client1->start();
  client2->start();
  client1->join();
  client2->join();

  //ASSERT
  int *rdma_buffer = (int *)m_nodeServer->getBuffer(0);

  //Assert 3 segments was created
  const size_t expectedSegments = 3;

  std::cout << "Buffer " << '\n';
  for (int i = 0; i < Config::DPI_SEGMENT_SIZE/sizeof(int)*3; i++) //Read 2 segments (3 is created but only the first 2 are filled)
  {
    std::cout << rdma_buffer[i] << ' '; 
    result.push_back(rdma_buffer[i]); //Somehow ignore the header... or add the header to the expected result
  }
  CPPUNIT_ASSERT_EQUAL(expectedSegments, m_stub_regClient->dpi_retrieve_buffer(bufferName)->segments.size());

  CPPUNIT_ASSERT_EQUAL(expectedSegments, m_stub_regClient->dpi_retrieve_buffer(bufferName)->segments.size());

  //Insert header to expected (order does not matter since we sort it)  
  for(size_t i = 0; i < 2; i++)
  {
    expectedResult.push_back(Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t));
    expectedResult.push_back(0);
    expectedResult.push_back(1);//Should have followingSegment
    expectedResult.push_back(0);
  }
  

  std::sort(expectedResult.begin(), expectedResult.end());
  std::sort(result.begin(), result.end());

  CPPUNIT_ASSERT_EQUAL(expectedResult.size()*2, result.size());
  
  for(size_t i = 0; i < expectedResult.size(); i++)
  {
    CPPUNIT_ASSERT_EQUAL(expectedResult[i], result[i]);
  }
} 