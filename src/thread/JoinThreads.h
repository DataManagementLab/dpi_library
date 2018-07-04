

#ifndef SRC_THREAD_JOINTHREADS_H_
#define SRC_THREAD_JOINTHREADS_H_

namespace dpi {

#include <vector>
#include <thread>

class JoinThreads {

 public:
    explicit JoinThreads(std::vector<Thread*> &threads) : m_threads(threads) {};

    virtual ~JoinThreads() {
        for (auto &thread : m_threads) {
            std::cout << "Desutrcuted thread" << std::endl;
            if (thread->running()) {
                thread->stop();
                thread->join();
            }
        }

    }

private:

    std::vector<Thread*> &m_threads;
};



}  // namespace dpi



#endif /* SRC_THREAD_JOINTHREADS_H_ */
