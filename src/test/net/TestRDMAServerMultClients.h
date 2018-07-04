

#ifndef SRC_TEST_NET_TestRDMAServerMultClients_H_
#define SRC_TEST_NET_TestRDMAServerMultClients_H_

#include "../../utils/Config.h"
#include "../../net/rdma/RDMAServer.h"
#include "../../net/rdma/RDMAClient.h"


using namespace dpi;

class TestRDMAServerMultClients : public CppUnit::TestFixture {
ISTORE_UNIT_TEST_SUITE (TestRDMAServerMultClients);
  ISTORE_UNIT_TEST_RC(testSendRecieve);ISTORE_UNIT_TEST_UD (testSendRecieve);ISTORE_UNIT_TEST_SUITE_END ()
  ;

 public:
  void setUp();
  void tearDown();

  void testSendRecieve();

 private:
  RDMAServer* m_rdmaServer;
  RDMAClient* m_rdmaClient_0;
  RDMAClient* m_rdmaClient_1;
  RDMAClient* m_rdmaClient_2;
  RDMAClient* m_rdmaClient_3;

  string m_connection;

  struct testMsg {
  int id;
  char a;
  testMsg(int n, char t)
      : id(n),
        a(t)  // Create an object of type _book.
  {
  };
};
};

#endif /* SRC_TEST_NET_TESTDATANODE_H_ */
