

#include "DPIAppendBenchmark.h"

mutex DPIAppendBenchmark::waitLock;
condition_variable DPIAppendBenchmark::waitCv;
bool DPIAppendBenchmark::signaled;

DPIAppendBenchmarkThread::DPIAppendBenchmarkThread(string &conns,
                                           size_t size, size_t iter)
{
  m_size = size;
  m_iter = iter;
  m_conns = conns;
  string bufferName = "appendBenchmark";
  BufferHandle *buffHandle = new BufferHandle(bufferName, 1);
  m_regClient = new RegistryClient();
  
  m_regClient->createBuffer(bufferName, 1, (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)),Config::DPI_SEGMENT_SIZE * Config::DPI_SEGMENT_THRESHOLD_FACTOR);

  m_bufferWriter = new BufferWriter<BufferWriterPrivate>(buffHandle, Config::DPI_SCRATCH_PAD_SIZE, m_regClient);
  std::cout << "DPI append Thread constructed"  << '\n';
  
}

DPIAppendBenchmarkThread::~DPIAppendBenchmarkThread()
{
  
}

void DPIAppendBenchmarkThread::run()
{

  std::cout << "DPI append Thread running"  << '\n';
  unique_lock<mutex> lck(DPIAppendBenchmark::waitLock);
  if (!DPIAppendBenchmark::signaled)
  {
    m_ready = true;
    DPIAppendBenchmark::waitCv.wait(lck);
  }
  lck.unlock();

  std::cout << "Benchmark Started" << '\n';
  int *scratchPad = (int *)m_bufferWriter->getScratchPad();
  
  scratchPad[0] = 1;
  scratchPad[1] = 2;
  scratchPad[2] = 2;
  scratchPad[3] = 2;
  scratchPad[4] = 2;
  scratchPad[5] = 2;
  scratchPad[6] = 2;
  scratchPad[7] = 2;


  startTimer();
  for (size_t i = 0; i <= m_iter; i++)
  {
    m_bufferWriter->appendFromScratchpad(sizeof(int)*8);
  }
  endTimer();
}


DPIAppendBenchmark::DPIAppendBenchmark(config_t config, bool isClient)
    : DPIAppendBenchmark(config.server, config.port, config.data, config.iter,
                     config.threads)
{
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
  m_size = sizeof(int);
  m_iter = iter;
  m_numThreads = threads;
  DPIAppendBenchmark::signaled = false;

  std::cout << "Iterations in client" << m_iter << '\n';
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
    // std::cout << "Buffer" << '\n';
    // int* buffer = (int* )m_nodeServer->getBuffer(2048);
    // std::cout << "buffer[8]" << buffer[1] << '\n';
    // std::cout << "buffer[8]" << buffer[2] << '\n';
    // std::cout << "buffer[8]" << buffer[3] << '\n';
    // std::cout << "buffer[8]" << buffer[4] << '\n';
    // std::cout << "buffer[8]" << buffer[5] << '\n';
    // std::cout << "buffer[8]" << buffer[6] << '\n';
    // std::cout << "buffer[8]" << buffer[7] << '\n';
    // std::cout << "buffer[8]" << buffer[8] << '\n';
    // std::cout << "buffer[8]" << buffer[9] << '\n';
    // std::cout << "buffer[8]" << buffer[10] << '\n';
    // std::cout << "buffer[8]" << buffer[11] << '\n';
    // std::cout << "buffer[8]" << buffer[12] << '\n';
    // std::cout << "buffer[8]" << buffer[13] << '\n';
  }
}

void DPIAppendBenchmark::runClient()
{
  //start all client threads
  for (size_t i = 0; i < m_numThreads; i++)
  {
    DPIAppendBenchmarkThread *perfThread = new DPIAppendBenchmarkThread(m_conns,
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
