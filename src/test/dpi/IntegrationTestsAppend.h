/**
 * @file IntegrationTestsAppend.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#pragma once

#include "../../utils/Config.h"

#include "../../dpi/RegistryClient.h"
#include "../../dpi/RegistryServer.h"
#include "../../dpi/BufferHandle.h"
#include "../../dpi/NodeServer.h"
#include "../../dpi/NodeClient.h"
#include "../../dpi/RegistryClient.h"
#include "../../dpi/BufferWriter.h"

#include <atomic>


class IntegrationTestsAppend : public CppUnit::TestFixture
{
  DPI_UNIT_TEST_SUITE(IntegrationTestsAppend);
    DPI_UNIT_TEST(SimpleIntegrationWithAppendInts_BW);
    DPI_UNIT_TEST(FourAppendersConcurrent_BW);
    DPI_UNIT_TEST(SimpleAppendAndConsume_LAT);
    DPI_UNIT_TEST(FourAppendersConcurrent_LAT);
  DPI_UNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void SimpleIntegrationWithAppendInts_BW();
  void FourAppendersConcurrent_BW();
  void SimpleAppendAndConsume_LAT();
  void FourAppendersConcurrent_LAT();

  static std::atomic<int> bar;    // Counter of threads, faced barrier.
  static std::atomic<int> passed; // Number of barriers, passed by all threads.

private:
  RegistryClient *m_regClient;
  RegistryServer *m_regServer;
  NodeServer *m_nodeServer; 
  NodeClient *m_nodeClient; 



template <class DataType> 
class BufferWriterClient : public Thread
{
  BufferHandle* buffHandle = nullptr;
  string& bufferName = "";
  std::vector<DataType> *dataToWrite = nullptr; //tuple<ptr to data, size in bytes>

public: 

  BufferWriterClient(string& bufferName, std::vector<DataType> *dataToWrite, int numThread = 4, BufferHandle::Buffertype buffertype = BufferHandle::Buffertype::BW) : 
    Thread(), bufferName(bufferName), dataToWrite(dataToWrite), NUMBER_THREADS(numThread), buffertype(buffertype) {}

  int NUMBER_THREADS;
  BufferHandle::Buffertype buffertype;
  void run() 
  {
    //ARRANGE
    BufferWriter *buffWriter = nullptr;
    if (buffertype == BufferHandle::Buffertype::LAT)
    {
      buffWriter = new BufferWriterLat(bufferName, new RegistryClient(), Config::DPI_INTERNAL_BUFFER_SIZE);
    }
    else
    {
      buffWriter = new BufferWriterBW(bufferName, new RegistryClient(), Config::DPI_INTERNAL_BUFFER_SIZE);
    }

    barrier_wait(); //Use barrier to simulate concurrent appends between BufferWriters

    //ACT
    for(size_t i = 0; i < dataToWrite->size(); i++)
    {
      buffWriter->append(&dataToWrite->operator[](i), sizeof(DataType));
    }

    buffWriter->close();
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