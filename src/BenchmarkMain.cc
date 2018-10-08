

#include "./utils/Config.h"
#include "./benchmark/BenchmarkRunner.h"
#include "./benchmark/Benchmarks.h"

#define no_argument 0
#define required_argument 1
#define optional_argument 2


static void printUsage() {
  cout << "dpi_bench -n #testNum options" << endl;

  cout << "Benchmarks:" << endl;
  cout << "1: \t DPIAppendClient" << endl;
  cout << "2: \t DPIAppendServer" << endl;  
}

BenchmarkRunner* createTest(config_t& config) {
  BenchmarkRunner* test;
  switch (config.number) {
    case 1:
      test = new DPIAppendBenchmark(config, true);
      break;
    case 2:
      test = new DPIAppendBenchmark(config, false);
      break;
    default:
      printUsage();
      exit(1);
    break;
  }

  return test;
}
int main(int argc, char* argv[]) {
  // load configuration
  string prog_name = string(argv[0]);
  static Config conf(prog_name);

  // parse parameters
  struct config_t config = BenchmarkRunner::parseParameters(argc, argv);

  // check if test number is defined
  if (config.number <= 0) {
    printUsage();
    return -1;
  }

  // create test and check if test is runnable
  BenchmarkRunner* test = createTest(config);
  if (!test->isRunnable()) {
    test->printUsage();
    return -1;
  }

  // run test client or server
  if (test->isClient()) {
    test->runClient();
    test->printHeader();
    test->printResults();
  } else {
    test->runServer();
  }
  delete test;

  //done
  return 1;
}
