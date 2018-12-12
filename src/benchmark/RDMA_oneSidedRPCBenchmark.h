/**
 * @file RDMAOneRPCBench.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */



#ifndef OneSidedRPC
#define OneSidedRPC

#include "../utils/Config.h"
#include "../utils/StringHelper.h"
#include "../thread/Thread.h"
#include "../net/rdma/RDMAClient.h"
#include "../dpi/NodeServer.h"
#include "../dpi/RegistryServer.h"
#include "../dpi/BufferWriter.h"
#include "BenchmarkRunner.h"

#include <vector>
#include <mutex>
#include <condition_variable>
#include <iostream>


namespace dpi
{

class RDMA_OneRPCThread : public Thread
{
public:
  RDMA_OneRPCThread(NodeID nodeid,string &conns, size_t size, size_t iter);
  ~RDMA_OneRPCThread();
  void run();
  bool ready()
  {
    return m_ready;
  }

private:
  bool m_ready = false;
  BufferWriter<BufferWriterPrivate>* m_bufferWriter = nullptr; 
  RegistryClient* m_regClient = nullptr;
  void *m_data;
  size_t m_size;
  size_t m_iter;
  string m_conns;
  NodeID m_nodeId;
  int m_ctr = 0;
};


class RDMAOneRPCBench : public BenchmarkRunner
{
public:
  RDMAOneRPCBench(config_t config, bool isClient);

  RDMAOneRPCBench(string &region, size_t serverPort, size_t size, size_t iter,
                 size_t threads);

  ~RDMAOneRPCBench();

  void printHeader()
  {
    cout << "Size\tIter\tBW (MB)\tmopsPerS" << endl;
  }

  void printResults()
  {
    double time = (this->time()) / (1e9);
    size_t bw = (((double)m_size * m_iter * m_numThreads) / (1024 * 1024)) / time;
    double mops = (((double)m_iter * m_numThreads) / time) / (1e6);

    cout << m_size << "\t" << m_iter << "\t" << bw << "\t" << mops << endl;
  }

  void printUsage()
  {
    cout << "istore2_perftest ... -s #servers ";
    cout << "(-d #dataSize -p #serverPort -t #threadNum)?" << endl;
  }

  void runClient();
  void runServer();
  double time();

  static mutex waitLock;
  static condition_variable waitCv;
  static bool signaled;

private:

  size_t m_serverPort;
  size_t m_size;
  size_t m_iter;
  size_t m_numThreads;
  string m_conns;
  vector<RDMA_OneRPCThread *> m_threads;

  RDMAServer* m_dServer = nullptr;

};

} // namespace dpi

#endif /* OneSidedRPC*/
