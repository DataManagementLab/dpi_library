#include "TestBufferConsumer.h"
#include <chrono>

std::atomic<int> TestBufferConsumer::bar{0};    // Counter of threads, faced barrier.
std::atomic<int> TestBufferConsumer::passed{0}; // Number of barriers, passed by all threads.

void TestBufferConsumer::setUp()
{  
  //Setup Test DPI
  Config::RDMA_MEMSIZE = 1024ul * 1024 * 1024 * 1;  //1GB
  Config::DPI_SEGMENT_SIZE = (512 + sizeof(Config::DPI_SEGMENT_HEADER_t));
  Config::DPI_INTERNAL_BUFFER_SIZE = 1024;

  //BENCHMARK STUFF!!!
  // Config::RDMA_MEMSIZE = 1024ul * 1024 * 1024 * 10;  //1GB
  // Config::DPI_SEGMENT_SIZE = (80 * 1048576 + sizeof(Config::DPI_SEGMENT_HEADER_t)); //80 MiB
  // Config::DPI_INTERNAL_BUFFER_SIZE = 1024 * 1024 * 8;


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

void TestBufferConsumer::tearDown()
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

void TestBufferConsumer::AppendAndConsumeNotInterleaved_ReuseSegs()
{
  //ARRANGE
  string bufferName = "buffer1";
    
  // size_t remoteOffset = 0;
  size_t memSize = sizeof(int);

  size_t numberSegments = 4;
  size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / memSize * numberSegments;
  BufferHandle *buffHandle = new BufferHandle(bufferName, 1, numberSegments, true); //Create 1 less segment in ring to test BufferWriter creating a segment on the ring
  DPI_DEBUG("Created BufferHandle\n");
  m_regClient->registerBuffer(buffHandle);
  DPI_DEBUG("Registered Buffer in Registry\n");
  buffHandle = m_regClient->createSegmentRingOnBuffer(bufferName);
  DPI_DEBUG("Created segment ring on buffer\n");


  // int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);
  // for(size_t i = 0; i < numberSegments*Config::DPI_SEGMENT_SIZE/memSize; i++)
  // {
  //   std::cout << rdma_buffer[i] << " ";
  // } std::cout << std::endl;

  BufferWriter buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_regClient);
  BufferConsumer buffConsumer(bufferName, m_regClient, m_nodeServer);

  for (size_t i = 0; i < numberElements; i++)
  {
    CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter, (void *)&i, memSize));
  }

  CPPUNIT_ASSERT(buffWriter.close());
  
  // for(size_t i = 0; i < numberSegments*Config::DPI_SEGMENT_SIZE/memSize; i++)
  // {
  //   if (i % (Config::DPI_SEGMENT_SIZE/memSize) == 0)
  //     std::cout << std::endl;
  //   std::cout << rdma_buffer[i] << " ";
  // } std::cout << std::endl;
  
  //ACT & ASSERT
  size_t i = 0;
  int expected = 0;
  size_t segmentSize = 0;
  int* segment = (int*)buffConsumer.consume(segmentSize);  
  int* lastSegment;
  while (segment != nullptr)
  {
    ++i;

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Segment size did not match expected", Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t), segmentSize);
    for (uint32_t j = 0; j < segmentSize/memSize; j++)
    {
      CPPUNIT_ASSERT_EQUAL_MESSAGE("Data value did not match", expected, segment[j]);
      expected++;
    }
    lastSegment = segment;
    segment = (int*)buffConsumer.consume(segmentSize);  

    //Assert header on last segment
    Config::DPI_SEGMENT_HEADER_t* header = (Config::DPI_SEGMENT_HEADER_t*)((char*)lastSegment - sizeof(Config::DPI_SEGMENT_HEADER_t));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Counter did not reset", (uint64_t)0, header->counter);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("CanConsume did not match expected", false, Config::DPI_SEGMENT_HEADER_FLAGS::getCanConsumeSegment(header->segmentFlags));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("CanWrite did not match expected", true, Config::DPI_SEGMENT_HEADER_FLAGS::getCanWriteToSegment(header->segmentFlags));
  }
  CPPUNIT_ASSERT_EQUAL(numberSegments, i);

  // for(size_t i = 0; i < numberSegments*Config::DPI_SEGMENT_SIZE/memSize; i++)
  // {
  //   std::cout << rdma_buffer[i] << " ";
  // } std::cout << std::endl;
}



void TestBufferConsumer::FourAppendersOneConsumerInterleaved_DontReuseSegs()
{
  //ARRANGE
  string bufferName = "test";
  int nodeId = 1;

  size_t segPerClient = 3;
  int64_t numberElements = segPerClient * (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int64_t); 
  std::vector<int64_t> *dataToWrite = new std::vector<int64_t>();
  
  for(int64_t i = 0; i < numberElements; i++)
  {
    dataToWrite->push_back(i);
  }  

  CPPUNIT_ASSERT(m_regClient->registerBuffer(new BufferHandle(bufferName, nodeId, segPerClient, true)));
  BufferHandle *buffHandle1 = m_regClient->createSegmentRingOnBuffer(bufferName);
  BufferHandle *buffHandle2 = m_regClient->createSegmentRingOnBuffer(bufferName);
  BufferHandle *buffHandle3 = m_regClient->createSegmentRingOnBuffer(bufferName);
  BufferHandle *buffHandle4 = m_regClient->createSegmentRingOnBuffer(bufferName);

  // int64_t *rdma_buffer = (int64_t *)m_nodeServer->getBuffer();

  // std::cout << "Buffer before appending" << '\n';
  // for (size_t i = 0; i < Config::DPI_SEGMENT_SIZE/sizeof(int64_t)*segPerClient*4; i++)
  // {
  //   if (i % (Config::DPI_SEGMENT_SIZE/sizeof(int64_t)) == 0)
  //     std::cout << std::endl;
  //   std::cout << rdma_buffer[i] << ' ';
  // }

  BufferWriterClient<int64_t> *client1 = new BufferWriterClient<int64_t>(buffHandle1, dataToWrite);
  BufferWriterClient<int64_t> *client2 = new BufferWriterClient<int64_t>(buffHandle2, dataToWrite);
  BufferWriterClient<int64_t> *client3 = new BufferWriterClient<int64_t>(buffHandle3, dataToWrite);
  BufferWriterClient<int64_t> *client4 = new BufferWriterClient<int64_t>(buffHandle4, dataToWrite);

  //ACT & ASSERT
  auto nodeServer = m_nodeServer;
  auto regClient = m_regClient;
  bool *runConsumer = new bool(true);
  size_t* segmentsConsumed = new size_t(0);
  std::thread consumer([nodeServer, &bufferName, &runConsumer, segmentsConsumed, regClient](){
    BufferConsumer buffConsumer(bufferName, regClient, nodeServer);
    size_t segmentSize = 0;
    int* lastSegment;
    while (*runConsumer)
    {
      int* segment = (int*)buffConsumer.consume(segmentSize);
      while (segment != nullptr)
      {
        ++(*segmentsConsumed);
        // std::cout << "Offset: " << (char*)segment - (char*)rdma_buffer << '\n';
        if (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t) != segmentSize)
        {
          std::cout << "SEGMENT SIZE DID NOT MATCH COUNTER!!! counter: " << segmentSize << '\n';
          // int64_t *rdma_buffer = (int64_t *)segment;
          // for (uint32_t j = 0; j < segmentSize/sizeof(int64_t); j++)
          // {
          //   std::cout << rdma_buffer[j] << ' ';
          // }
          break;
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Segment size did not match expected", Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t), segmentSize);
        // for (uint32_t j = 0; j < segmentSize/sizeof(int64_t); j++)
        // {
        //   CPPUNIT_ASSERT_EQUAL_MESSAGE("Data value did not match", expected, segment[j]);
        //   expected++;
        // }
        lastSegment = segment;
        segment = (int*)buffConsumer.consume(segmentSize);  

        //Assert header on last segment
        Config::DPI_SEGMENT_HEADER_t* header = (Config::DPI_SEGMENT_HEADER_t*)((char*)lastSegment - sizeof(Config::DPI_SEGMENT_HEADER_t));
        auto offsetStr = to_string((char*)lastSegment - (char*)nodeServer->getBuffer());
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Counter did not reset, on offset: " + offsetStr, (uint64_t)0, header->counter);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("CanConsume did not match expected on offset: " + offsetStr, false, Config::DPI_SEGMENT_HEADER_FLAGS::getCanConsumeSegment(header->segmentFlags));
        CPPUNIT_ASSERT_EQUAL_MESSAGE("CanWrite did not match expected on offset: " + offsetStr, true, Config::DPI_SEGMENT_HEADER_FLAGS::getCanWriteToSegment(header->segmentFlags));
      }
      usleep(100);
    }    
  });

  client1->start();
  client2->start();
  client3->start();
  client4->start();

  client1->join();
  client2->join();
  client3->join();
  client4->join();
  *runConsumer = false;
  consumer.join();

  // std::cout << "Buffer after appending, total int64_t's: " << numberElements << " * 4" << '\n';
  // for (size_t i = 0; i < Config::DPI_SEGMENT_SIZE/sizeof(int64_t)*segPerClient*4; i++)
  // {
  //   if (i % (Config::DPI_SEGMENT_SIZE/sizeof(int64_t)) == 0)
  //     std::cout << std::endl;
  //   std::cout << rdma_buffer[i] << ' ';
  // }
  CPPUNIT_ASSERT_EQUAL((size_t)segPerClient * 4, (size_t)*segmentsConsumed);

  // m_nodeServer->localFree(rdma_buffer);
}



void TestBufferConsumer::FourAppendersOneConsumerInterleaved_ReuseSegs()
{
  //ARRANGE
  string bufferName = "test";
  int nodeId = 1;

  size_t segsPerRing = 3; //Only create 3 segments per ring
  size_t segPerClient = 6; //Each appender wants to write 6 segments
  int64_t numberElements = segPerClient * (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / sizeof(int64_t); 
  std::vector<int64_t> *dataToWrite = new std::vector<int64_t>();
  
  for(int64_t i = 0; i < numberElements; i++)
  {
    dataToWrite->push_back(i);
  }

  CPPUNIT_ASSERT(m_regClient->registerBuffer(new BufferHandle(bufferName, nodeId, segsPerRing, true)));
  BufferHandle *buffHandle1 = m_regClient->createSegmentRingOnBuffer(bufferName);
  BufferHandle *buffHandle2 = m_regClient->createSegmentRingOnBuffer(bufferName);
  BufferHandle *buffHandle3 = m_regClient->createSegmentRingOnBuffer(bufferName);
  BufferHandle *buffHandle4 = m_regClient->createSegmentRingOnBuffer(bufferName);

  // int64_t *rdma_buffer = (int64_t *)m_nodeServer->getBuffer();

  // std::cout << "Buffer before appending" << '\n';
  // for (size_t i = 0; i < Config::DPI_SEGMENT_SIZE/sizeof(int64_t)*segsPerRing*4; i++)
  // {
  //   if (i % (Config::DPI_SEGMENT_SIZE/sizeof(int64_t)) == 0)
  //     std::cout << std::endl;
  //   std::cout << rdma_buffer[i] << ' ';
  // }

  BufferWriterClient<int64_t> *client1 = new BufferWriterClient<int64_t>(buffHandle1, dataToWrite);
  BufferWriterClient<int64_t> *client2 = new BufferWriterClient<int64_t>(buffHandle2, dataToWrite);
  BufferWriterClient<int64_t> *client3 = new BufferWriterClient<int64_t>(buffHandle3, dataToWrite);
  BufferWriterClient<int64_t> *client4 = new BufferWriterClient<int64_t>(buffHandle4, dataToWrite);

  //ACT & ASSERT
  auto nodeServer = m_nodeServer;
  auto regClient = m_regClient;
  bool *runConsumer = new bool(true);
  size_t* segmentsConsumed = new size_t(0);
  std::thread consumer([nodeServer, &bufferName, &runConsumer, segmentsConsumed, regClient](){
    BufferConsumer buffConsumer(bufferName, regClient, nodeServer);

    size_t segmentSize = 0;
    int* lastSegment;
    while (*runConsumer)
    {
      int* segment = (int*)buffConsumer.consume(segmentSize);
      while (segment != nullptr)
      {
        ++(*segmentsConsumed);
        // std::cout << "Offset: " << (char*)segment - (char*)rdma_buffer << '\n';
        if (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t) != segmentSize)
        {
          std::cout << "SEGMENT SIZE DID NOT MATCH COUNTER!!! counter: " << segmentSize << '\n';
          // int64_t *rdma_buffer = (int64_t *)segment;
          // for (uint32_t j = 0; j < segmentSize/sizeof(int64_t); j++)
          // {
          //   std::cout << rdma_buffer[j] << ' ';
          // }
          break;
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Segment size did not match expected", Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t), segmentSize);
        // for (uint32_t j = 0; j < segmentSize/sizeof(int64_t); j++)
        // {
        //   CPPUNIT_ASSERT_EQUAL_MESSAGE("Data value did not match", expected, segment[j]);
        //   expected++;
        // }
        lastSegment = segment;
        segment = (int*)buffConsumer.consume(segmentSize);

        //Assert header on last segment
        Config::DPI_SEGMENT_HEADER_t* header = (Config::DPI_SEGMENT_HEADER_t*)((char*)lastSegment - sizeof(Config::DPI_SEGMENT_HEADER_t));
        auto offsetStr = to_string((char*)lastSegment - (char*)nodeServer->getBuffer());
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Counter did not reset, on offset: " + offsetStr, (uint64_t)0, header->counter);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("CanConsume did not match expected on offset: " + offsetStr, false, Config::DPI_SEGMENT_HEADER_FLAGS::getCanConsumeSegment(header->segmentFlags));
        CPPUNIT_ASSERT_EQUAL_MESSAGE("CanWrite did not match expected on offset: " + offsetStr, true, Config::DPI_SEGMENT_HEADER_FLAGS::getCanWriteToSegment(header->segmentFlags));
      }
      usleep(100);
    }    
  });

  client1->start();
  client2->start();
  client3->start();
  client4->start();

  client1->join();
  client2->join();
  client3->join();
  client4->join();
  *runConsumer = false;
  consumer.join();

  // std::cout << "Buffer after appending, total int64_t's: " << numberElements << " * 4" << '\n';
  // for (size_t i = 0; i < Config::DPI_SEGMENT_SIZE/sizeof(int64_t)*segsPerRing*4; i++)
  // {
  //   if (i % (Config::DPI_SEGMENT_SIZE/sizeof(int64_t)) == 0)
  //     std::cout << std::endl;
  //   std::cout << rdma_buffer[i] << ' ';
  // }
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Consumed number of segments did not match expected", (size_t)segPerClient * 4, (size_t)*segmentsConsumed);

  // m_nodeServer->localFree(rdma_buffer);
}



void TestBufferConsumer::AppenderConsumerBenchmark()
{
  //ARRANGE

  string bufferName = "test";
  int nodeId = 1;

  size_t segsPerRing = 20; //Only create segsPerRing segments per ring
  size_t segPerClient = 100; //Each appender wants to write segPerClient segments

  size_t dataSize = 1024 * 8;

  void *scratchPad = malloc(dataSize);
  memset(scratchPad, 1, dataSize);

  CPPUNIT_ASSERT(m_regClient->registerBuffer(new BufferHandle(bufferName, nodeId, segsPerRing, true)));

  BufferHandle *buffHandle = m_regClient->createSegmentRingOnBuffer(bufferName);
  BufferWriter buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, new RegistryClient());
  


  //ACT & ASSERT

  std::cout << "Starting consumer" << '\n';

  auto nodeServer = m_nodeServer;
  auto regClient = m_regClient;
  bool *runConsumer = new bool(true);
  size_t* segmentsConsumed = new size_t(0);
  std::thread consumer([nodeServer, &bufferName, &runConsumer, segmentsConsumed, regClient](){
    BufferConsumer buffConsumer(bufferName, regClient, nodeServer);
    size_t segmentSize = 0;
    int* lastSegment;
    while (*runConsumer)
    {
      int* segment = (int*)buffConsumer.consume(segmentSize);
      while (segment != nullptr)
      {
        ++(*segmentsConsumed);
        // std::cout << "Offset: " << (char*)segment - (char*)rdma_buffer << '\n';
        if (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t) != segmentSize)
        {
          std::cout << "SEGMENT SIZE DID NOT MATCH COUNTER!!! counter: " << segmentSize << '\n';
          // int64_t *rdma_buffer = (int64_t *)segment;
          // for (uint32_t j = 0; j < segmentSize/sizeof(int64_t); j++)
          // {
          //   std::cout << rdma_buffer[j] << ' ';
          // }
          break;
        }
        // CPPUNIT_ASSERT_EQUAL_MESSAGE("Segment size did not match expected", Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t), segmentSize);
        // for (uint32_t j = 0; j < segmentSize/sizeof(int64_t); j++)
        // {
        //   CPPUNIT_ASSERT_EQUAL_MESSAGE("Data value did not match", expected, segment[j]);
        //   expected++;
        // }
        lastSegment = segment;
        segment = (int*)buffConsumer.consume(segmentSize);

        //Assert header on last segment
        Config::DPI_SEGMENT_HEADER_t* header = (Config::DPI_SEGMENT_HEADER_t*)((char*)lastSegment - sizeof(Config::DPI_SEGMENT_HEADER_t));
        auto offsetStr = to_string((char*)lastSegment - (char*)nodeServer->getBuffer());
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Counter did not reset, on offset: " + offsetStr, (uint64_t)0, header->counter);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("CanConsume did not match expected on offset: " + offsetStr, false, Config::DPI_SEGMENT_HEADER_FLAGS::getCanConsumeSegment(header->segmentFlags));
        CPPUNIT_ASSERT_EQUAL_MESSAGE("CanWrite did not match expected on offset: " + offsetStr, true, Config::DPI_SEGMENT_HEADER_FLAGS::getCanWriteToSegment(header->segmentFlags));
        // usleep(100000);
      }
    }    
  });

  auto startTime = chrono::high_resolution_clock::now();

  uint64_t numberElements = segPerClient * (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / dataSize; 

  std::cout << "Starting to append" << '\n';

  for (size_t i = 0; i < numberElements; i++)
  {
    buffWriter.append(scratchPad, dataSize);
  }

  buffWriter.close();

  auto endTime = chrono::high_resolution_clock::now();

  auto diff = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);

  *runConsumer = false;
  consumer.join();

  std::cout << "Time: " << diff.count() << " ms" << '\n';

  CPPUNIT_ASSERT_EQUAL_MESSAGE("Consumed number of segments did not match expected", (size_t)segPerClient, (size_t)*segmentsConsumed);

}