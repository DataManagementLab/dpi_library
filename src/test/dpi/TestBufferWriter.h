#pragma once

#include "../../utils/Config.h"

#include "../../dpi/NodeServer.h" 
#include "../../dpi/NodeClient.h"
#include "../../dpi/RegistryClient.h"
#include "../../dpi/BufferWriter.h"

#include <atomic>

class TestBufferWriter : public CppUnit::TestFixture {
DPI_UNIT_TEST_SUITE(TestBufferWriter);
  DPI_UNIT_TEST(testAppendPrivate_WithScratchpad);
  DPI_UNIT_TEST(testAppendPrivate_WithoutScratchpad);
  DPI_UNIT_TEST(testAppendPrivate_MultipleClients_WithScratchpad); 
  DPI_UNIT_TEST(testAppendPrivate_SizeTooBigForScratchpad);
  DPI_UNIT_TEST(testBuffer);
  DPI_UNIT_TEST(testAppendPrivate_WithoutScratchpad_splitData);
  DPI_UNIT_TEST(testAppendPrivate_MultipleConcurrentClients);    
  DPI_UNIT_TEST(testAppendShared_AtomicHeaderManipulation);
  DPI_UNIT_TEST(testAppendShared_MultipleConcurrentClients);  
DPI_UNIT_TEST_SUITE_END();

 public:
  void setUp();
  void tearDown();

  // Private Strategy
  void testAppendPrivate_WithScratchpad();
  void testAppendPrivate_WithoutScratchpad_splitData();
  void testAppendPrivate_WithoutScratchpad();
  void testAppendPrivate_MultipleClients_WithScratchpad();
  void testAppendPrivate_SizeTooBigForScratchpad();
  void testAppendPrivate_MultipleConcurrentClients();
  void testBuffer();

  // Shared Strategy
  void testAppendShared_AtomicHeaderManipulation();
  void testAppendShared_WithScratchpad();
  void testAppendShared_MultipleConcurrentClients();


  static std::atomic<int> bar;    // Counter of threads, faced barrier.
  static std::atomic<int> passed; // Number of barriers, passed by all threads.
  static const int NUMBER_THREADS = 2;


private:
  NodeClient* m_nodeClient;
  NodeServer* m_nodeServer;
  RegistryClient* m_stub_regClient;

class RegistryClientStub : public RegistryClient
{
public:

  BuffHandle* dpi_create_buffer(string& name, NodeID node_id, string& connection)
  {
    std::cout << "Created buffer" << '\n';
    (void) name;
    m_buffHandle = new BuffHandle(name, node_id, connection);
    return m_buffHandle;
  }

  bool dpi_register_buffer(BuffHandle* handle)
  {
    BuffHandle* copy_buffHandle = new BuffHandle(handle->name, handle->node_id, handle->connection);
    for(auto segment : handle->segments){
      copy_buffHandle->segments.push_back(segment); 
    }
    m_buffHandle = copy_buffHandle;
    return true;
  }
  BuffHandle* dpi_retrieve_buffer(string& name)
  {
    (void) name;
    //Copy a new BuffHandle to emulate distributed setting (Or else one nodes changes to the BuffHandle would affect another nodes BuffHandle without retrieving the buffer first)
    BuffHandle*  copy_buffHandle = new BuffHandle(m_buffHandle->name, m_buffHandle->node_id, m_buffHandle->connection);
    for(auto segment : m_buffHandle->segments){
      copy_buffHandle->segments.push_back(segment); 
    }
    return copy_buffHandle;
  }
  bool dpi_append_segment(string& name, BuffSegment& segment)
  {
    //Implement locking if stub should support concurrent appending of segments.
    (void) name;
    appendSegMutex.lock();
    BuffSegment seg;
    seg.offset = segment.offset;
    seg.size = segment.size;
    seg.threshold = segment.threshold;
    m_buffHandle->segments.push_back(seg);
    appendSegMutex.unlock();
    
    return true;
  }

private:
  BuffHandle* m_buffHandle = nullptr; //For this stub we just have one buffHandle
  std::mutex appendSegMutex;
};
 
struct TestData
{
  int a;
  int b;
  int c;
  int d;
  TestData(int a, int b, int c, int d) : a(a), b(b), c(c), d(d){}
};

template <class DataType, class Strategy>
class BufferWriterClient : public Thread
{
  NodeServer* nodeServer = nullptr;
  RegistryClient* regClient = nullptr;
  std::vector<DataType> *dataToWrite = nullptr; //tuple<ptr to data, size in bytes>
  BuffHandle* buffHandle = nullptr;

public: 

  BufferWriterClient(NodeServer* nodeServer, RegistryClient* regClient, BuffHandle* buffHandle, std::vector<DataType> *dataToWrite) : 
    Thread(), nodeServer(nodeServer), regClient(regClient), buffHandle(buffHandle), dataToWrite(dataToWrite) {}

  void run() 
  {
    //ARRANGE
    string bufferName = "test";
    string connection = "127.0.0.1:5400";

    BufferWriter<Strategy> buffWriter(buffHandle, Config::DPI_SCRATCH_PAD_SIZE, regClient);

    barrier_wait();

    //ACT
    for(int i = 0; i < dataToWrite->size(); i++)
    {
      buffWriter.append(&dataToWrite->operator[](i), sizeof(DataType));
    }
  }

    void barrier_wait()
    {
        std::cout << "Enter Barrier" << '\n';
        int passed_old = passed.load(std::memory_order_relaxed);

        if (bar.fetch_add(1) == (NUMBER_THREADS - 1))
        {
            // The last thread, faced barrier.
            bar = 0;
            // Synchronize and store in one operation.
            passed.store(passed_old + 1, std::memory_order_release);
        }
        else
        {
            // Not the last thread. Wait others.
            while (passed.load(std::memory_order_relaxed) == passed_old)
            {
            };
            // Need to synchronize cache with other threads, passed barrier.
            std::atomic_thread_fence(std::memory_order_acquire);
        }
    }
};

};
