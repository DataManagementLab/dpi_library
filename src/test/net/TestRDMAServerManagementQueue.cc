
#include "TestRDMAServerManagementQueue.h"

void TestRDMAServerManagementQueue::setUp() {
  m_rdmaServer = new RDMAServer();
  CPPUNIT_ASSERT(m_rdmaServer->startServer());

  m_connection = "127.0.0.1:" + to_string(Config::RDMA_PORT);
  m_rdmaClient = new RDMAClient();
  CPPUNIT_ASSERT(m_rdmaClient->connect(m_connection, true));
}

void TestRDMAServerManagementQueue::tearDown() {
  if (m_rdmaServer != nullptr) {
    m_rdmaServer->stopServer();
    delete m_rdmaServer;
    m_rdmaServer = nullptr;
  }
  if (m_rdmaClient != nullptr) {
    delete m_rdmaClient;
    m_rdmaClient = nullptr;
  }
}

void TestRDMAServerManagementQueue::testWrite() {
  //create local array
  int* localValues = (int*) m_rdmaClient->localAlloc(2 * sizeof(int));
  CPPUNIT_ASSERT(localValues != nullptr);

  //write to remote machine
  size_t remoteOffset = 0;
  localValues[0] = 1;
  localValues[1] = 2;
  CPPUNIT_ASSERT(
      m_rdmaClient->write(m_connection, remoteOffset, localValues, sizeof(int) * 2, true));

  //read from remote machine
  int* remoteVals = (int*) m_rdmaServer->getBuffer(remoteOffset);
  CPPUNIT_ASSERT_EQUAL(remoteVals[0], localValues[0]);
  CPPUNIT_ASSERT_EQUAL(remoteVals[1], localValues[1]);
}

void TestRDMAServerManagementQueue::testSendRecieve() {
  //management message
  int* managementRec = (int*) m_rdmaClient->localAlloc(sizeof(int));
  int* managementSend = (int*) m_rdmaServer->localAlloc(sizeof(int));
  (*managementSend) = 101;

  //real message
  testMsg* localstruct = (testMsg*) m_rdmaClient->localAlloc(sizeof(testMsg));
  CPPUNIT_ASSERT(localstruct!=nullptr);
  localstruct->a = 'a';
  localstruct->id = 1;
  testMsg* remotestruct = (testMsg*) m_rdmaServer->localAlloc(sizeof(testMsg));
  CPPUNIT_ASSERT(remotestruct!=nullptr);

  //connections
  ib_addr_t serverQP = m_rdmaServer->getQueues()[0];
  ib_addr_t clientMgmtQP = m_rdmaClient->getMgmtQueue(m_connection);
  ib_addr_t serverMgmtQP = m_rdmaServer->getMgmtQueue(serverQP);

  //send data over normal queues
  CPPUNIT_ASSERT(
      m_rdmaServer->receive(serverQP, (void* ) remotestruct, sizeof(testMsg)));
  CPPUNIT_ASSERT(
      m_rdmaClient->send(m_connection, (void*) localstruct, sizeof(testMsg), false));

  //send back over mgmt queue
  CPPUNIT_ASSERT(
      m_rdmaClient->receive(clientMgmtQP, (void* ) managementRec, sizeof(int)));
  CPPUNIT_ASSERT(
      m_rdmaServer->send(serverMgmtQP, (void*) managementSend, sizeof(int), false));

  //poll for completion on both sides
  CPPUNIT_ASSERT(m_rdmaServer->pollReceive(serverQP));
  CPPUNIT_ASSERT(m_rdmaClient->pollReceive(clientMgmtQP));

  //asert
  CPPUNIT_ASSERT_EQUAL(localstruct->id, remotestruct->id);
  CPPUNIT_ASSERT_EQUAL(*managementSend, *managementRec);
}
