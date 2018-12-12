

#include "RDMA_oneSidedRPCBenchmark.h"

mutex RDMAOneRPCBench::waitLock;
condition_variable RDMAOneRPCBench::waitCv;
bool RDMAOneRPCBench::signaled;

RDMA_OneRPCThread::RDMA_OneRPCThread(NodeID nodeid, string &conns,
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
  m_bufferWriter = new BufferWriter<BufferWriterPrivate>(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_regClient);
  std::cout << "DPI append Thread constructed" << '\n';
}

RDMA_OneRPCThread::~RDMA_OneRPCThread()
{
}

void RDMA_OneRPCThread::run()
{

  std::cout << "DPI append Thread running" << '\n';
  unique_lock<mutex> lck(RDMAOneRPCBench::waitLock);
  if (!RDMAOneRPCBench::signaled)
  {
    m_ready = true;
    RDMAOneRPCBench::waitCv.wait(lck);
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

RDMAOneRPCBench::RDMAOneRPCBench(config_t config, bool isClient)
    : RDMAOneRPCBench(config.server, config.port, config.data, config.iter,
                      config.threads)
{

  Config::DPI_NODES.clear();
  string dpiServerNode = config.server + ":" + to_string(config.port);
  Config::DPI_NODES.push_back(dpiServerNode);
  std::cout << "connection: " << Config::DPI_NODES.back() << '\n';
  Config::DPI_REGISTRY_SERVER = config.registryServer;
  Config::DPI_REGISTRY_PORT = config.registryPort;
  Config::DPI_INTERNAL_BUFFER_SIZE = config.internalBufSize;

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

RDMAOneRPCBench::RDMAOneRPCBench(string &conns, size_t serverPort,
                                 size_t size, size_t iter, size_t threads)
{
  m_conns = conns;
  m_serverPort = serverPort;
  m_size = size;
  m_iter = iter;
  m_numThreads = threads;
  RDMAOneRPCBench::signaled = false;

  std::cout << "Iterations in client" << m_iter << '\n';
  std::cout << "With " << m_size << " byte appends" << '\n';
}

RDMAOneRPCBench::~RDMAOneRPCBench()
{
  if (this->isClient())
  {
    for (size_t i = 0; i < m_threads.size(); i++)
    {
      delete m_threads[i];
    }
    m_threads.clear();
  } else {
    if (m_dServer != nullptr) {
      m_dServer->stopServer();
      delete m_dServer;
    }
    m_dServer = nullptr;
}
}

void RDMAOneRPCBench::runServer()
{
  std::cout << "Starting RDMA Server port " << m_serverPort << '\n';

  m_dServer = new RDMAServer(m_serverPort);
  if (!m_dServer->startServer())
  {
    throw invalid_argument("RemoteMemoryPerf could not start server!");
  }

  while (m_dServer->isRunning())
  {
    usleep(Config::DPI_SLEEP_INTERVAL);
  }
}

void RDMAOneRPCBench::runClient()
{
  //start all client threads
  for (size_t i = 1; i <= m_numThreads; i++)
  {
    RDMA_OneRPCThread *perfThread = new RDMA_OneRPCThread(i, m_conns,
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
  //waitForUser();
  usleep(10000000);

  //send signal to run benchmark
  RDMAOneRPCBench::signaled = false;
  unique_lock<mutex> lck(RDMAOneRPCBench::waitLock);
  RDMAOneRPCBench::waitCv.notify_all();
  RDMAOneRPCBench::signaled = true;
  lck.unlock();
  for (size_t i = 0; i < m_threads.size(); i++)
  {
    m_threads[i]->join();
  }
}

double RDMAOneRPCBench::time()
{
  uint128_t totalTime = 0;
  for (size_t i = 0; i < m_threads.size(); i++)
  {
    totalTime += m_threads[i]->time();
  }
  return ((double)totalTime) / m_threads.size();
}
