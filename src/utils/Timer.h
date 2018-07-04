

#ifndef SRC_UTILS_TIMER_H_
#define SRC_UTILS_TIMER_H_

#include "../utils/Config.h"

#include <unordered_map>

namespace istore2 {

class Timer {

 public:
  inline static uint128_t timestamp() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec * 1e9 + ts.tv_nsec;
  }

  inline static uint128_t diff(uint128_t last) {
    uint128_t now = timestamp();
    return now - last;
  }

  void addTime(string step, uint128_t time) {
    if (m_stats.find(step) != m_stats.end()) {
      m_stats[step] += time;
    } else {
      m_stats[step] = time;
    }
  }

  void addTimer(Timer* timer) {
    if (timer == nullptr)
    return;

    for (unordered_map<string, uint128_t>::iterator it =
        timer->m_stats.begin(); it != timer->m_stats.end(); ++it) {
      if (m_stats.find(it->first) != m_stats.end()) {
        m_stats[it->first] += it->second;
      } else {
        m_stats[it->first] = it->second;
      }
    }
  }

  unordered_map<string, uint128_t> getStats() {
    return m_stats;
  }

private:
  unordered_map<string, uint128_t> m_stats;
};

}

#endif /* SRC_UTILS_TIMER_H_ */
