

#include "DPIAppendBenchmark.h"

mutex DPIAppendBenchmark::waitLock;
condition_variable DPIAppendBenchmark::waitCv;
bool DPIAppendBenchmark::signaled;

DPIAppendBenchmarkThread::DPIAppendBenchmarkThread(NodeID nodeid, string &conns,
                                                   size_t size, size_t iter)
{
  m_size = size;
  m_iter = iter;
  m_nodeId = nodeid;
  m_conns = conns;
  
  string bufferName = "appendBenchmark";
  BufferHandle *buffHandle;
  m_regClient = new RegistryClient();

  if (m_nodeId == 1)
  {
    buffHandle = m_regClient->createBuffer(bufferName, m_nodeId, (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)), Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR);
  }
  else
  {
    usleep(100000);
    buffHandle = m_regClient->retrieveBuffer(bufferName);
  }
  m_bufferWriter = new BufferWriter<BufferWriterShared>(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_regClient);
  std::cout << "DPI append Thread constructed" << '\n';
}

DPIAppendBenchmarkThread::~DPIAppendBenchmarkThread()
{
}

void DPIAppendBenchmarkThread::run()
{

  std::cout << "DPI append Thread running" << '\n';
  unique_lock<mutex> lck(DPIAppendBenchmark::waitLock);
  if (!DPIAppendBenchmark::signaled)
  {
    m_ready = true;
    DPIAppendBenchmark::waitCv.wait(lck);
  }
  lck.unlock();

  std::cout << "Benchmark Started" << '\n';
  void *scratchPad = malloc(m_size);
  memset(scratchPad, 1, m_size);

  startTimer();
  for (size_t i = 0; i <= m_iter; i++)
  {
    m_bufferWriter->append(scratchPad, m_size);
  }

  m_bufferWriter->close();
  endTimer();
}

DPIAppendBenchmark::DPIAppendBenchmark(config_t config, bool isClient)
    : DPIAppendBenchmark(config.server, config.port, config.data, config.iter,
                         config.threads)
{

  Config::DPI_NODES.clear();
  string dpiServerNode = config.server + ":" + to_string(config.port);
  Config::DPI_NODES.push_back(dpiServerNode);
  std::cout << "connection: " << Config::DPI_NODES.back() << '\n';
  Config::DPI_REGISTRY_SERVER = config.registryServer;
  Config::DPI_REGISTRY_PORT = config.registryPort;

  this->isClient(isClient);

  //check parameters
  if (isClient && config.server.length() > 0)
  {
    std::cout << "Starting DPIAppend Clients" << '\n';
    this->isRunnable(true);
  }
  else if (!isClient)
  {
    this->isRunnable(true);
  }
}

DPIAppendBenchmark::DPIAppendBenchmark(string &conns, size_t serverPort,
                                       size_t size, size_t iter, size_t threads)
{
  m_conns = conns;
  m_serverPort = serverPort;
  m_size = size;
  m_iter = iter;
  m_numThreads = threads;
  DPIAppendBenchmark::signaled = false;

  std::cout << "Iterations in client" << m_iter << '\n';
  std::cout << "With " << m_size << " byte appends" << '\n';
}

DPIAppendBenchmark::~DPIAppendBenchmark()
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

void DPIAppendBenchmark::runServer()
{
  std::cout << "Starting DPI Server port " << m_serverPort << '\n';

  m_nodeServer = new NodeServer(m_serverPort);

  if (!m_nodeServer->startServer())
  {
    throw invalid_argument("DPIAppendBenchmark could not start NodeServer!");
  }

  m_regServer = new RegistryServer();

  if (!m_regServer->startServer())
  {
    throw invalid_argument("DPIAppendBenchmark could not start RegistryServer!");
  }

  while (m_nodeServer->isRunning())
  {
    usleep(Config::DPI_SLEEP_INTERVAL);
  }
}

void DPIAppendBenchmark::runClient()
{
  //start all client threads
  for (size_t i = 1; i <= m_numThreads; i++)
  {
    DPIAppendBenchmarkThread *perfThread = new DPIAppendBenchmarkThread(i, m_conns,
                                                                        m_size,
                                                                        m_iter);
    perfThread->start();
    if (!perfThread->ready())
    {
      usleep(Config::DPI_SLEEP_INTERVAL);
    }
    m_threads.push_back(perfThread);
  }

  //wait for user input
  waitForUser();

  //send signal to run benchmark
  DPIAppendBenchmark::signaled = false;
  unique_lock<mutex> lck(DPIAppendBenchmark::waitLock);
  DPIAppendBenchmark::waitCv.notify_all();
  DPIAppendBenchmark::signaled = true;
  lck.unlock();
  for (size_t i = 0; i < m_threads.size(); i++)
  {
    m_threads[i]->join();
  }
}

double DPIAppendBenchmark::time()
{
  uint128_t totalTime = 0;
  for (size_t i = 0; i < m_threads.size(); i++)
  {
    totalTime += m_threads[i]->time();
  }
  return ((double)totalTime) / m_threads.size();
}
