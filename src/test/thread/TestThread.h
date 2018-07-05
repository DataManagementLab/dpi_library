

#ifndef SRC_TEST_THREAD_TESTTHREAD_H_
#define SRC_TEST_THREAD_TESTTHREAD_H_

#include "../../utils/Config.h"
#include "../../thread/Thread.h"

using namespace dpi;

class TestThread : public CppUnit::TestFixture {
DPI_UNIT_TEST_SUITE(TestThread);
  DPI_UNIT_TEST(testRun);
  DPI_UNIT_TEST_SUITE_END();
 private:
  class TestingThread : public Thread {
   public:
    bool runned = false;
    void run() {
      runned = true;
    }

  };
  TestingThread* m_testingThread;

 public:
  void setUp();
  void tearDown();
  void testRun();
};

#endif /* SRC_TEST_THREAD_TESTTHREAD_H_ */
