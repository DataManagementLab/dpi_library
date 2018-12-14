/**
 * @file TestBufferWriter.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include "../../utils/Config.h"

#include "../../dpi/NodeServer.h" 
#include "../../dpi/NodeClient.h"
#include "../../dpi/RegistryClient.h"
#include "../../dpi/RegistryServer.h"
#include "../../dpi/BufferWriter.h"
#include "RegistryClientStub.h"

#include <atomic>

class TestBufferWriter : public CppUnit::TestFixture {
DPI_UNIT_TEST_SUITE(TestBufferWriter);

  DPI_UNIT_TEST(testBuffer);
  DPI_UNIT_TEST(testAppend_SimpleData);
  DPI_UNIT_TEST(testAppend_SingleInts);
  DPI_UNIT_TEST(testAppend_SplitData);
  DPI_UNIT_TEST(testAppend_MultipleConcurrentClients);
  DPI_UNIT_TEST(testAppend_VaryingDataSizes);
DPI_UNIT_TEST_SUITE_END();
 
 public:
  void setUp();
  void tearDown();
 
  void testBuffer();
  void testAppend_SingleInts();
  void testAppend_SplitData();
  void testAppend_SimpleData();
  void testAppend_MultipleConcurrentClients();
  void testAppend_VaryingDataSizes();

  static std::atomic<int> bar;    // Counter of threads, faced barrier.
  static std::atomic<int> passed; // Number of barriers, passed by all threads.
  static const int NUMBER_THREADS = 2;


private:
  void* readSegmentData(BufferSegment* segment, size_t &size);
  void* readSegmentData(size_t offset, size_t &size);
  Config::DPI_SEGMENT_HEADER_t *readSegmentHeader(size_t offset);
  Config::DPI_SEGMENT_HEADER_t *readSegmentHeader(BufferSegment* segment);

  RDMAClient* m_rdmaClient;
  NodeServer* m_nodeServer;
  RegistryClient* m_regClient;
  RegistryServer *m_regServer;

 
struct TestData
{
  int a;
  int b;
  int c;
  int d;
  TestData(int a, int b, int c, int d) : a(a), b(b), c(c), d(d){}
};

template <class DataType>
class BufferWriterClient : public Thread
{
  NodeServer* nodeServer = nullptr;
  RegistryClient* regClient = nullptr;
  BufferHandle* buffHandle = nullptr;
  string& buffername = "";
  std::vector<DataType> *dataToWrite = nullptr; //tuple<ptr to data, size in bytes>

public: 

  BufferWriterClient(NodeServer* nodeServer, string& bufferName, std::vector<DataType> *dataToWrite) : 
    Thread(), nodeServer(nodeServer), buffername(bufferName), dataToWrite(dataToWrite) {}

  void run() 
  {
    //ARRANGE

    BufferWriterBW buffWriter(buffername, new RegistryClient(), Config::DPI_INTERNAL_BUFFER_SIZE);

    barrier_wait();

    //ACT
    for(size_t i = 0; i < dataToWrite->size(); i++)
    {
      buffWriter.append(&dataToWrite->operator[](i), sizeof(DataType));
    }

    buffWriter.close();
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
