

#include "DPILatencyBenchmark.h"

mutex DPILatencyBenchmark::waitLock;
condition_variable DPILatencyBenchmark::waitCv;
bool DPILatencyBenchmark::signaled;

DPILatencyBenchmarkThread::DPILatencyBenchmarkThread(NodeID nodeid, string &conns,
                                                     size_t size, size_t iter, size_t numberAppenders, bool signaled)
{
  m_size = size;
  m_iter = iter;
  m_nodeId = nodeid;
  m_conns = conns;
  m_sendSignaled = signaled;
  std::cout << "/* m_size */" << m_size << '\n';
  std::cout << "/* iterations */" << iter << '\n';
  std::cout << "Sending Signaled " << m_sendSignaled << '\n';

  BufferHandle *buffHandle;
  m_regClient = new RegistryClient();

  if (m_nodeId == 1)
  {
    buffHandle = new BufferHandle(bufferName, m_nodeId, numberSegments, numberAppenders, m_size, BufferHandle::Buffertype::LAT);
    m_regClient->registerBuffer(buffHandle);
  }
  else
  {
    usleep(100000);
  }
  m_bufferWriter = new BWriter(bufferName, m_regClient, Config::DPI_INTERNAL_BUFFER_SIZE);
  std::cout << "DPI " << benchmark << "Append Thread constructed" << '\n';
}

DPILatencyBenchmarkThread::~DPILatencyBenchmarkThread()
{
}

void DPILatencyBenchmarkThread::run()
{

  std::cout << "DPI append Thread running" << '\n';
  unique_lock<mutex> lck(DPILatencyBenchmark::waitLock);
  if (!DPILatencyBenchmark::signaled)
  {
    m_ready = true;
    DPILatencyBenchmark::waitCv.wait(lck);
  }
  lck.unlock();

  std::cout << "Benchmark Started" << '\n';
  void *scratchPad = malloc(m_size);
  memset(scratchPad, 1, m_size);

  startTimer();
  for (size_t i = 0; i <= m_iter; i++)
  {
    m_bufferWriter->append(scratchPad, m_size, m_sendSignaled);
  }

  m_bufferWriter->close();

  endTimer();
}

DPILatencyBenchmark::DPILatencyBenchmark(config_t config, bool isClient)
    : DPILatencyBenchmark(config.server, config.port, config.data, config.iter,
                          config.threads)
{

  Config::DPI_NODES.clear();
  string dpiServerNode = config.server + ":" + to_string(config.port);
  Config::DPI_NODES.push_back(dpiServerNode);
  std::cout << "connection: " << Config::DPI_NODES.back() << '\n';
  Config::DPI_REGISTRY_SERVER = config.registryServer;
  Config::DPI_REGISTRY_PORT = config.registryPort;
  Config::DPI_INTERNAL_BUFFER_SIZE = config.internalBufSize;

  m_sendSignaled = config.signaled;

  this->isClient(isClient);

  //check parameters
  if (isClient && config.server.length() > 0)
  {
    std::cout << "Starting DPIAppend Clients" << '\n';
    std::cout << "Internal buf size: " << Config::DPI_INTERNAL_BUFFER_SIZE << '\n';
    this->isRunnable(true);
  }
  else if (!isClient)
  {
    this->isRunnable(true);
  }
}

DPILatencyBenchmark::DPILatencyBenchmark(string &conns, size_t serverPort,
                                         size_t size, size_t iter, size_t threads)
{
  m_conns = conns;
  m_serverPort = serverPort;
  m_size = size;
  m_iter = iter;
  m_numThreads = threads;
  DPILatencyBenchmark::signaled = false;

  std::cout << "Iterations in client" << m_iter << '\n';
  std::cout << "With " << m_size << " byte appends" << '\n';
}

DPILatencyBenchmark::~DPILatencyBenchmark()
{
  if (this->isClient())
  {
    for (size_t i = 0; i < m_threads.size(); i++)
    {
      delete m_threads[i];
    }
    m_threads.clear();
  }
  else
  {
    if (m_nodeServer != nullptr)
    {
      std::cout << "stopping node server" << '\n';
      m_nodeServer->stopServer();
      delete m_nodeServer;
    }
    m_nodeServer = nullptr;

    if (m_regServer != nullptr)
    {
      m_regServer->stopServer();
      delete m_regServer;
    }
    m_regServer = nullptr;
  }
}

void DPILatencyBenchmark::runServer()
{
  std::cout << "Starting DPI Server port " << m_serverPort << '\n';

  m_nodeServer = new NodeServer(m_serverPort);

  if (!m_nodeServer->startServer())
  {
    throw invalid_argument("DPILatencyBenchmark could not start NodeServer!");
  }

  m_regServer = new RegistryServer();

  if (!m_regServer->startServer())
  {
    throw invalid_argument("DPILatencyBenchmark could not start RegistryServer!");
  }

  auto m_regClient = new RegistryClient();
  usleep(Config::DPI_SLEEP_INTERVAL);

  ;

  while (nullptr == m_regClient->retrieveBuffer(bufferName))
  {
    usleep(Config::DPI_SLEEP_INTERVAL * 5);
  }

  auto handle_ret = m_regClient->retrieveBuffer(bufferName);


  BIter bufferIterator = handle_ret->getIteratorLat((char *)m_nodeServer->getBuffer());
  DPILatencyBenchmarkBarrier barrier((char *)m_nodeServer->getBuffer(), handle_ret->entrySegments,bufferName);
  
  barrier.create();
  std::cout << "Client Barrier created" << '\n';
  waitForUser();
  barrier.release();
  int count = 0;
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

  // bufferIterator.has_next();
  // std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  while (bufferIterator.has_next())
  {
    // std::cout << "Interator Segment" << '\n';
    size_t dataSize;
    int *data = (int *)bufferIterator.next(dataSize);
    for (size_t i = 0; i < dataSize / m_size; i++, data++)
    {
      count++;
    }
  }
  std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  std::cout << "Benchmark took " << duration << "microseconds" << std::endl;
  std::cout << "Number of msgs " << count << std::endl;
  std::cout << "Latency " << (double)( (double) duration / (double)count) << std::endl;
  std::cout << "Mops " << (((double)count) / duration ) << std::endl;

}

void DPILatencyBenchmark::runClient()
{
  //start all client threads
  for (size_t i = 1; i <= m_numThreads; i++)
  {
    DPILatencyBenchmarkThread *perfThread = new DPILatencyBenchmarkThread(i, m_conns,
                                                                          m_size,
                                                                          m_iter, m_numThreads, m_sendSignaled);
    perfThread->start();
    if (!perfThread->ready())
    {
      usleep(Config::DPI_SLEEP_INTERVAL);
    }
    m_threads.push_back(perfThread);
  }

  //wait for user input
  waitForUser();
  // usleep(10000000);

  //send signal to run benchmark
  DPILatencyBenchmark::signaled = false;
  unique_lock<mutex> lck(DPILatencyBenchmark::waitLock);
  DPILatencyBenchmark::waitCv.notify_all();
  DPILatencyBenchmark::signaled = true;
  lck.unlock();
  for (size_t i = 0; i < m_threads.size(); i++)
  {
    m_threads[i]->join();
  }
}

double DPILatencyBenchmark::time()
{
  uint128_t totalTime = 0;
  for (size_t i = 0; i < m_threads.size(); i++)
  {
    totalTime += m_threads[i]->time();
  }
  return ((double)totalTime) / m_threads.size();
}
