#include "TestNodeClient.h"
#include "../../net/rdma/RDMAClient.h"

void TestNodeClient::setUp()
{

  m_nodeClient = new NodeClient();
  m_regClient = new RegistryClient();
  m_nodeServer = new NodeServer();
  CPPUNIT_ASSERT(m_nodeServer->startServer());
};

void TestNodeClient::tearDown()
{
  delete m_nodeClient;
  delete m_regClient;
  if (m_nodeServer->running())
  {
    m_nodeServer->stopServer();
  }
  delete m_nodeServer;
};

void TestNodeClient::testRemoteAlloc()
{
  string connection = "127.0.0.1:5400";
  RDMAClient *m_rdmaClient = new RDMAClient();
  size_t nodeid = 1;
  m_rdmaClient->connect(connection, nodeid);
  int *buff = (int *)m_rdmaClient->localAlloc(sizeof(int));

  for (size_t i = 0; i < 5; i++)
  {
    size_t remoteOffset = 0;
    CPPUNIT_ASSERT(m_rdmaClient->remoteAlloc(connection, Config::DPI_SEGMENT_SIZE, remoteOffset));
    std::cout << "RemoteOffset: " << remoteOffset << '\n';
    for (int j = 0; j < 512; j++)
    {
      memcpy(buff, &j, sizeof(int));
      m_rdmaClient->write(nodeid, remoteOffset + j * sizeof(int), (void *)buff, sizeof(int), true);
    }
  }

  int *rdma_buffer = (int *)m_nodeServer->getBuffer(0);

  DebugCode(
      std::cout << "Buffer " << '\n';
      for (int i = 0; i < 5 * 512; i++)
          std::cout
      << ((int *)rdma_buffer)[i] << ' ';);
}

void TestNodeClient::testBuffer()
{

  size_t numberElements = 5000;
  int *rdma_buffer = (int *)m_nodeServer->getBuffer(0);
  for (int i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(rdma_buffer[i] == 0);
  }
}

void TestNodeClient::testAppendShared_WithoutScratchpad()
{
//ARRANGE
  string bufferName = "test";
  string connection = "127.0.0.1:5400";
  size_t remoteOffset = 0;
  size_t memSize = sizeof(int);

  size_t numberElements = 1200;

  BuffHandle *buffHandle = new BuffHandle(bufferName, 1, connection);
  BufferWriter<BufferWriterPrivate> buffWriter(buffHandle);

//ACT
  for (int i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(buffWriter.append((void *)&i, memSize));
  }

  int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);

  DebugCode(
      std::cout << "Buffer " << '\n';
      for (int i = 0; i < numberElements; i++)
          std::cout
      << rdma_buffer[i] << ' ';);

//ASSERT
  for (int i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(rdma_buffer[i] == i);
  }

  // CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter,(void*) &data, sizeof(int)));
};


void TestNodeClient::testAppendShared_WithScratchpad()
{
//ARRANGE
  string bufferName = "test";
  string connection = "127.0.0.1:5400";
  size_t remoteOffset = 0;
  size_t memSize = sizeof(int);

  size_t numberElements = Config::DPI_SCRATCH_PAD_SIZE/sizeof(int)*2;

  BuffHandle *buffHandle = new BuffHandle(bufferName, 1, connection);
  BufferWriter<BufferWriterPrivate> buffWriter(buffHandle);

  int* scratchPad = (int*) buffWriter.getScratchPad();

//ACT
  //Fill ScratchPad and append when it is full or last iteration
  int scratchIter = 0;
  for (int i = 0; i <= numberElements; i++)
  {
    if ((i % (Config::DPI_SCRATCH_PAD_SIZE/sizeof(int))) == 0 && i > 0)
    {
      std::cout << "appending at " << i << std::endl;
      CPPUNIT_ASSERT(buffWriter.appendFromScratchpad(Config::DPI_SCRATCH_PAD_SIZE));
      scratchIter = 0;
    }
    scratchPad[scratchIter] = i;
    scratchIter++;
  }

  int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);

  DebugCode(
    std::cout << "Buffer " << '\n';
    for (int i = 0; i < numberElements; i++)
        std::cout << rdma_buffer[i] << ' ';
  );

//ASSERT
  for (int i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(rdma_buffer[i] == i);
  }

  // CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter,(void*) &data, sizeof(int)));
};


void TestNodeClient::testAppendShared_MultipleClients_WithScratchpad()
{
//ARRANGE
  string bufferName = "test";
  string connection = "127.0.0.1:5400";

  //Client 1
  size_t numberElements1 = Config::DPI_SCRATCH_PAD_SIZE/sizeof(int)*2;

  BuffHandle *buffHandle1 = new BuffHandle(bufferName, 1, connection);
  BufferWriter<BufferWriterPrivate> buffWriter1(buffHandle1);

  int* scratchPad1 = (int*) buffWriter1.getScratchPad();

  //Client 2
  size_t numberElements2 = Config::DPI_SCRATCH_PAD_SIZE/sizeof(int)*2;

  BuffHandle *buffHandle2 = new BuffHandle(bufferName, 1, connection);
  BufferWriter<BufferWriterPrivate> buffWriter2(buffHandle2);

  int* scratchPad2 = (int*) buffWriter2.getScratchPad();

//ACT
  //Client 1 append
  //Fill ScratchPad and append when it is full
  int scratchIter1 = 0;
  for (int i = 0; i <= numberElements1; i++)
  {
    if ((i % (Config::DPI_SCRATCH_PAD_SIZE/sizeof(int))) == 0 && i > 0)
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
  for (int i = 0; i <= numberElements2; i++)
  {
    if ((i % (Config::DPI_SCRATCH_PAD_SIZE/sizeof(int))) == 0 && i > 0)
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
  for (int i = 0; i < numberElements1; i++)
  {
    CPPUNIT_ASSERT_EQUAL(i, rdma_buffer[i]);
  }

  //Client 2
  for (int i = 0, j = numberElements1; i < numberElements1; i++, j++)
  {
    CPPUNIT_ASSERT_EQUAL(i, rdma_buffer[j]);
  }
};


void TestNodeClient::testAppendShared_SizeTooBigForScratchpad()
{
//ARRANGE
  string bufferName = "test";
  string connection = "127.0.0.1:5400";

  BuffHandle *buffHandle = new BuffHandle(bufferName, 1, connection);
  BufferWriter<BufferWriterPrivate> buffWriter(buffHandle, Config::DPI_SCRATCH_PAD_SIZE);

//ACT
//ASSERT
  CPPUNIT_ASSERT_MESSAGE("appendFromScratchpad should return false when size is bigger than scratchpad", 
    !buffWriter1.appendFromScratchpad(Config::DPI_SCRATCH_PAD_SIZE + 1));
}

void TestNodeClient::testAppendShared_SizeTooBigForScratchpad()
{
//ARRANGE
  string bufferName = "test";
  string connection = "127.0.0.1:5400";

  BuffHandle *buffHandle = new BuffHandle(bufferName, 1, connection);
  BufferWriter<BufferWriterPrivate> buffWriter(buffHandle, Config::DPI_SCRATCH_PAD_SIZE);

//ACT
//ASSERT
  CPPUNIT_ASSERT_MESSAGE("appendFromScratchpad should return false when size is bigger than scratchpad", 
    !buffWriter1.appendFromScratchpad(Config::DPI_SCRATCH_PAD_SIZE + 1));
}