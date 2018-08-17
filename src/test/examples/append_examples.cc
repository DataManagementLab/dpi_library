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
  DPI_Context context;
  DPI_Init(context);
  DPI_Create_buffer(buffer_name, 1, context);
  DPI_Append(buffer_name, (void*)data, sizeof(data), context);

  size_t buffer_size;
  void* buffer_ptr;
  DPI_Close_buffer(buffer_name, context);
  DPI_Get_buffer(buffer_name, buffer_size, buffer_ptr, context);

  for(size_t i = 0; i < buffer_size; i++)
  {
    cout << ((char*)buffer_ptr)[i];
    CPPUNIT_ASSERT_EQUAL(data[i], ((char*)buffer_ptr)[i]);
  }
  DPI_Finalize(context);
}

void AppendExamples::paperExample()
{
  string buffer_name = "buffer";
  char data1[] = "Hello ";
  char data2[] = "World!";
  int rcv_node_id = 1; //ID is mapped to a concrete node in cluster spec
  DPI_Context context;
  DPI_Init(context); 
  DPI_Create_buffer(buffer_name, rcv_node_id, context);
  DPI_Append(buffer_name, (void*)data1, sizeof(data1), context);
  DPI_Append(buffer_name, (void*)data2, sizeof(data2), context);
  DPI_Close_buffer(buffer_name, context);

  size_t buffer_size;
  void* buffer_ptr;
  DPI_Get_buffer(buffer_name, buffer_size, buffer_ptr, context);

  for(size_t i = 0; i < buffer_size; i++)
  {
    cout << ((char*)buffer_ptr)[i];
  }
  DPI_Finalize(context);
}