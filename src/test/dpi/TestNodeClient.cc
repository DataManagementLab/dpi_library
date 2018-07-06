#include "TestNodeClient.h"
#include "../../net/rdma/RDMAClient.h"

  void TestNodeClient::setUp(){

    m_nodeClient = new NodeClient();
    m_regClient = new RegistryClient();
    m_nodeServer = new NodeServer();
    CPPUNIT_ASSERT(m_nodeServer->startServer());
  };

  void TestNodeClient::tearDown(){
    delete m_nodeClient;
    delete m_regClient;
    if(m_nodeServer->running()){
      m_nodeServer->stopServer();
    }
    delete m_nodeServer;
  };
  void TestNodeClient::testAppendShared(){

      string bufferName = "test";
      string connection = "10.116.60.16:5400";
      size_t remoteOffset = 0;
      size_t memSize = sizeof(int);

      size_t numberElements = 1200;

      BuffHandle* buffHandle = new BuffHandle(bufferName, 1, connection);
      BufferWriter<BufferWriterPrivate> buffWriter(buffHandle);

      for(int i = 0; i < numberElements; i++)
      {
        CPPUNIT_ASSERT( buffWriter.append((void*) &i, memSize));  
      }

      int* rdma_buffer = (int*) m_nodeServer->getBuffer(remoteOffset);
      
      DebugCode(
      std::cout << "Buffer "  << '\n';
      for(int i = 0; i < numberElements; i++)
        std::cout <<  ((int*) rdma_buffer)[i] << ' ';
      );

       for(int i = 0; i < numberElements; i++)
      {
        CPPUNIT_ASSERT( rdma_buffer[i] == i);  
      }
      
      // CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter,(void*) &data, sizeof(int)));
  };