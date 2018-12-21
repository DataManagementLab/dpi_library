/**
 * @file BenchmarkRunner.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-08-17
 */



#ifndef PERFTEST_H_
#define PERFTEST_H_

#include "../utils/Config.h"

#include <getopt.h>
#include <math.h>

namespace dpi {

struct config_t {
  size_t number = 0;
  string server = Config::DPI_REGISTRY_SERVER;
  size_t port = Config::DPI_NODE_PORT;
  string registryServer = Config::DPI_REGISTRY_SERVER;
  size_t registryPort = Config::DPI_REGISTRY_PORT;
  size_t iter = 100000;
  size_t threads = 1;
  size_t data = 2048;
  size_t internalBufSize = Config::DPI_INTERNAL_BUFFER_SIZE;
  bool signaled = false;
};

class BenchmarkRunner {
 public:
  static config_t parseParameters(int argc, char* argv[]) {
    // parse parameters
    struct config_t config;
    
    while (1) {
      struct option long_options[] = { { "number", required_argument, 0, 'n' },
          { "node server", optional_argument, 0, 's' },  { "port",
          optional_argument, 0, 'p' }, {"internal buffer size", optional_argument, 0, 'b'}, { "registry server", optional_argument, 0, 'r'}, {"registry port", optional_argument, 0, 'o'},
           { "data", optional_argument, 0, 'd' }, {"threads", optional_argument, 0, 't' }, {"iteration", optional_argument, 0, 'i' } , {"signaled", optional_argument, 0, 'a' } };

      int c = getopt_long(argc, argv, "n:d:s:t:p:i:r:o:b:a:", long_options, NULL);
      if (c == -1)
        break;

      switch (c) {
        case 'n':
          config.number = strtoul(optarg, NULL, 0);
          break;
        case 'd':
          config.data = strtoul(optarg, NULL, 0);
          break;
        case 's':
          config.server = string(optarg);
          break;
        case 'p':
          config.port = strtoul(optarg, NULL, 0);
          break;
        case 'r':
          config.registryServer = string(optarg);
          break;
        case 'o':
          config.registryPort = strtoul(optarg, NULL, 0);
          break;
        case 'b':
          std::cout << "hit case b" << '\n';
          config.internalBufSize = strtoul(optarg, NULL, 0);
          std::cout << "Updated internal buf size to " << config.internalBufSize << '\n';
          break;
        case 't':
          config.threads = strtoul(optarg, NULL, 0);
          break;
       case 'i':
          config.iter = strtoul(optarg, NULL, 0);
          break;
        case 'a':
          std::cout << "signaled " << config.signaled << '\n';
          size_t val = strtoul(optarg, NULL, 0);
          if(val){
              config.signaled = true;
          }else{
              config.signaled = false;
          }
          std::cout << "signaled " << config.signaled << '\n';
          break;
      }
    }
    return config;
  }

  virtual ~BenchmarkRunner() {
  }

  virtual void runClient()=0;
  virtual void runServer()=0;
  virtual double time()=0;

  virtual void printHeader()=0;
  virtual void printResults()=0;
  virtual void printUsage()=0;

  void isClient(bool isClient) {
    m_isClient = isClient;
  }

  void isRunnable(bool isRunnable) {
    m_isRunnable = isRunnable;
  }

  bool isClient() {
    return m_isClient;
  }

  bool isRunnable() {
    return m_isRunnable;
  }

  void waitForUser() {
    //wait for user input
    cout << "Press Enter to run Benchmark!" << flush << endl;
    char temp;
    cin.get(temp);
  }

 private:
  bool m_isClient = false;
  bool m_isRunnable = false;
};

}

#endif /* PERFTEST_H_ */
