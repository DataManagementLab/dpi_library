

#ifndef RemotePingPong_BW_H_
#define RemotePingPong_BW_H_

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

class DPIAppendBenchmarkThread : public Thread
{
public:
  DPIAppendBenchmarkThread(NodeID nodeid,string &conns, size_t size, size_t iter);
  ~DPIAppendBenchmarkThread();
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


class DPIAppendBenchmark : public BenchmarkRunner
{
public:
  DPIAppendBenchmark(config_t config, bool isClient);

  DPIAppendBenchmark(string &region, size_t serverPort, size_t size, size_t iter,
                 size_t threads);

  ~DPIAppendBenchmark();

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
  vector<DPIAppendBenchmarkThread *> m_threads;

  NodeServer *m_nodeServer;
  RegistryServer* m_regServer;

};

} // namespace dpi

#endif /* RemotePingPong_BW_H_*/
