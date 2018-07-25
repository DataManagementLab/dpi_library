#include "append_examples.h"

void AppendExamples::setUp()
{  
  //Setup Test DPI
  Config::RDMA_MEMSIZE = 1024ul * 1024 * 1024 * 1;  //1GB
  Config::DPI_SEGMENT_SIZE = (2048 + sizeof(Config::DPI_SEGMENT_HEADER_t));
  Config::DPI_INTERNAL_BUFFER_SIZE = 1024;
  Config::DPI_REGISTRY_SERVER = "127.0.0.1";
  Config::DPI_REGISTRY_PORT = 5300;
  Config::DPI_NODE_PORT = 5400;
  Config::DPI_NODES.clear();
  string dpiTestNode = "127.0.0.1:" + to_string(Config::DPI_NODE_PORT);
  Config::DPI_NODES.push_back(dpiTestNode);

  std::cout << "Starting NodeServer" << '\n';
  m_nodeServer = new NodeServer();
  CPPUNIT_ASSERT(m_nodeServer->startServer());
  std::cout << "NodeServer started" << '\n';
  std::cout << "Starting Registry Server" << '\n';
  m_regServer = new RegistryServer();
  CPPUNIT_ASSERT(m_regServer->startServer());
  std::cout << "Registry Server started" << '\n';
}

void AppendExamples::tearDown()
{
  if (m_regServer != nullptr)
  {
    m_regServer->stopServer();
    delete m_regServer;
    m_regServer = nullptr;
  }  
  if (m_nodeServer != nullptr)
  {
    m_nodeServer->stopServer();
    delete m_nodeServer;
    m_nodeServer = nullptr;
  }
}

void AppendExamples::example1()
{
  string buffer_name = "example";
  char data[] = "Hello World!";
  CONTEXT context;
  DPI_Init(context);
  DPI_Create_buffer(buffer_name, 1, context);
  DPI_Append(buffer_name, (void*)data, sizeof(data), context);
  DPI_Finalize(context);

  char* buffer = (char*)m_nodeServer->getBuffer(0);
  string hello(buffer + sizeof(Config::DPI_SEGMENT_HEADER_t), sizeof(data));
  string expected(data, sizeof(data));
  std::cout << hello << " == " << expected << '\n';
  CPPUNIT_ASSERT_EQUAL(expected,  hello);
}