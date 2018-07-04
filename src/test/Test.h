

#ifndef MYTEST_H_
#define MYTEST_H_

namespace dpi {

class Test {
 public:
  virtual ~Test() {
  }
  ;

  virtual int test()=0;
};

}

#endif /* MYTEST_H_ */
