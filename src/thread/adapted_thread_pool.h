

#ifndef SRC_THREAD_ADAPTED_THREAD_POOL_H_
#define SRC_THREAD_ADAPTED_THREAD_POOL_H_

#include <memory>
#include <algorithm>
#include <thread>
#include <future>
#include "threadsafe_queue.h"
#include "JoinThreads.h"
#include "../db/utils/LockFreeQueue.h"
#include "Thread.h"


namespace istore2{


template <typename INPUT, typename RESULT >
class adapted_thread_pool {


 protected:
     std::atomic_bool done;
     threadsafe_queue <INPUT> work_queue;
     LockFreeQueue<RESULT,1000>& completion_queue;
     std::vector<Thread*> threads;
     JoinThreads joiner;
     unsigned const thread_count;


 public:
     adapted_thread_pool(LockFreeQueue<RESULT,1000>& comp_qeue_) : done(false),completion_queue(comp_qeue_) ,joiner(threads), thread_count(Config::EXP_NUM_WORKER_THREADS) {
     }

     virtual void initThreadPool()=0;

     virtual ~adapted_thread_pool() {
         done = true;
     }

     void submit(INPUT input ){
         work_queue.push(input);
     }
 };


}



#endif /* SRC_THREAD_ADAPTED_THREAD_POOL_H_ */
