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

void TestNodeClient::testRemoteAlloc()
 {
   string connection = "127.0.0.1:5400";
   RDMAClient* m_rdmaClient = new RDMAClient();
   size_t nodeid = 1;
   m_rdmaClient->connect(connection, nodeid);
   int* buff = (int*) m_rdmaClient->localAlloc(sizeof(int));

   
   for(size_t i = 0; i < 5; i++)
   {
     size_t remoteOffset = 0;
     CPPUNIT_ASSERT(m_rdmaClient->remoteAlloc(connection, Config::DPI_SEGMENT_SIZE, remoteOffset));
     std::cout << "RemoteOffset: " << remoteOffset << '\n';
     for(int j = 0; j < 512; j++){
       memcpy(buff,&j,sizeof(int));
       m_rdmaClient->write(nodeid, remoteOffset + j*sizeof(int), (void*) buff, sizeof(int), true);
     }
   }

   int* rdma_buffer = (int*) m_nodeServer->getBuffer(0);
      
      DebugCode(
      std::cout << "Buffer "  << '\n';
      for(int i = 0; i < 5*512; i++)
        std::cout <<  ((int*) rdma_buffer)[i] << ' ';
      );

 }

void TestNodeClient::testBuffer(){

  size_t numberElements = 5000;
  int* rdma_buffer = (int*) m_nodeServer->getBuffer(0);
      for(int i = 0; i < numberElements; i++)
      {
        CPPUNIT_ASSERT( rdma_buffer[i] == 0);  
      }
}

  void TestNodeClient::testAppendShared(){

      string bufferName = "test";
      string connection = "127.0.0.1:5400";
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
        std::cout <<  rdma_buffer[i] << ' ';
      );


      for(int i = 0; i < numberElements; i++)
      {
        CPPUNIT_ASSERT( rdma_buffer[i] == i);  
      }

      
      
      // CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter,(void*) &data, sizeof(int)));
  };