

#ifndef SRC_TEST_NET_TestRDMAServerMCast_H_
#define SRC_TEST_NET_TestRDMAServerMCast_H_

#include "../../utils/Config.h"
#include "../../net/rdma/RDMAServer.h"
#include "../../net/rdma/RDMAClient.h"

using namespace dpi;

class TestRDMAServerMCast : public CppUnit::TestFixture {


ISTORE_UNIT_TEST_SUITE (TestRDMAServerMCast);
  ISTORE_UNIT_TEST_UD (testSendReceive);
  ISTORE_UNIT_TEST_UD (testSendReceiveWithIbAdress);
  ISTORE_UNIT_TEST_SUITE_END()
  ;

 public:
  void setUp();
  void tearDown();

  void testSendReceive();
  void testSendReceiveWithIbAdress();

 private:
  RDMAServer* m_rdmaServer;
  RDMAClient* m_rdmaClient;
  string m_mCastAddr;
  struct ib_addr_t m_addrClient;
  struct ib_addr_t m_addrServer;

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

#endif
