

#ifndef MYTEST_H_
#define MYTEST_H_

namespace istore2 {

class Test {
 public:
  virtual ~Test() {
  }
  ;

  virtual int test()=0;
};

}

#endif /* MYTEST_H_ */
