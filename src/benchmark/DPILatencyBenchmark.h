/**
 * @file DPILatencyBenchmark_H_.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */

#ifndef DPILatencyBenchmark_H_
#define DPILatencyBenchmark_H_

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

using BWriter = BufferWriterLat;
using BIter = BufferIteratorLat;
static string benchmark = "Latency";

class DPILatencyBenchmarkThread : public Thread
{

public:
  DPILatencyBenchmarkThread(NodeID nodeid, string &conns, size_t size, size_t iter, size_t numberAppenders);
  ~DPILatencyBenchmarkThread();
  void run();
  bool ready()
  {
    return m_ready;
  }

private:
  string bufferName = "buffer_benchmark";
  bool m_ready = false;
  BWriter *m_bufferWriter = nullptr;
  RegistryClient *m_regClient = nullptr;
  size_t numberSegments = 50;
  void *m_data;
  size_t m_size;
  size_t m_iter;
  string m_conns;
  NodeID m_nodeId;
  int m_ctr = 0;
};

class DPILatencyBenchmark : public BenchmarkRunner
{
public:
  DPILatencyBenchmark(config_t config, bool isClient);

  DPILatencyBenchmark(string &region, size_t serverPort, size_t size, size_t iter,
                      size_t threads);

  ~DPILatencyBenchmark();

  void printHeader()
  {
    cout << "Size\tIter\tBW (MB)\tmopsPerS\tTime (micro sec.)" << endl;
  }

  void printResults()
  {
    double time = (this->time()) / (1e9);
    size_t bw = (((double)m_size * m_iter * m_numThreads) / (1024 * 1024)) / time;
    double mops = (((double)m_iter * m_numThreads) / time) / (1e6);
    double ms_time = (this->time()) / (1e3);

    cout << m_size << "\t" << m_iter << "\t" << bw << "\t" << mops << "\t" << ms_time << endl;
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

  int numberSegments;

  string m_conns;
  string bufferName = "buffer_benchmark";

  vector<DPILatencyBenchmarkThread *> m_threads;


  NodeServer *m_nodeServer;
  RegistryServer *m_regServer;
};

} // namespace dpi

#endif /* RemotePingPong_BW_H_*/
