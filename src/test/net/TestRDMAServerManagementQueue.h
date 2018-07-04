

#ifndef SRC_TEST_NET_TestRDMAServerManagmentQueue_H_
#define SRC_TEST_NET_TestRDMAServerManagmentQueue_H_

#include "../../utils/Config.h"
#include "../../net/rdma/RDMAServer.h"
#include "../../net/rdma/RDMAClient.h"


using namespace istore2;

class TestRDMAServerManagementQueue : public CppUnit::TestFixture {
ISTORE_UNIT_TEST_SUITE (TestRDMAServerManagementQueue);
  ISTORE_UNIT_TEST_RC(testWrite);
  ISTORE_UNIT_TEST_RC(testSendRecieve);ISTORE_UNIT_TEST_UD (testSendRecieve);ISTORE_UNIT_TEST_SUITE_END()
  ;

 public:
  void setUp();
  void tearDown();

  void testWrite();
  void testSendRecieve();

 private:
  RDMAServer* m_rdmaServer;
  RDMAClient* m_rdmaClient;
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

#endif /* SRC_TEST_NET_TestRDMAServerManagmentQueue_H_ */
