

#ifndef SRC_TEST_NET_TestSimpleUD_H_
#define SRC_TEST_NET_TestSimpleUD_H_

#include "../../utils/Config.h"
#include "../../net/rdma/RDMAServer.h"
#include "../../net/rdma/RDMAClient.h"

using namespace dpi;

class TestSimpleUD : public CppUnit::TestFixture
{
    ISTORE_UNIT_TEST_SUITE(TestSimpleUD);
    ISTORE_UNIT_TEST_UD(testSendRecieve);
    ISTORE_UNIT_TEST_UD(testSendRecieveMgmt);
    ISTORE_UNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();

    void testSendRecieve();
    void testSendRecieveMgmt();

  private:
    RDMAServer *m_rdmaServer1;
    RDMAServer *m_rdmaServer2;

    RDMAClient *m_rdmaClient1;
    RDMAClient *m_rdmaClient2;

    string m_connection1;
    string m_connection2;

    struct testMsg
    {
        int id;
        char a;
        testMsg(int n, char t)
            : id(n),
              a(t) // Create an object of type _book.
              {};
    };
};

#endif
