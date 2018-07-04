

//CPPUnit
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>

#include <unistd.h>
#include <stdio.h>
#include <getopt.h>

#include "./utils/Config.h"
#include "./test/Tests.h"
#include "./test/Test.h"
#include "./thread/Thread.h"


#define no_argument 0
#define required_argument 1
#define optional_argument 2

static void usage() {
    cout << "dpi -r (test|sever) -n #number" << endl << endl;
    cout << "Tests:" << endl;
    cout << "1: \t thread/TestThread" << endl;
    cout << "2: \t thread/TestHelloServer" << endl;
    cout << "101: \t net/TestRDMAServer" << endl;
    cout << "102: \t net/TestRDMAServerMultipleClients" << endl;
    cout << "103: \t net/TestRDMAManagementQueue" << endl;
    cout << "104: \t net/TestSimpleUD" << endl;
    cout << "105: \t net/TestRDMAServerMCast" << endl;
    cout << endl;

    cout << "Server:" << endl;
}

static void runtest(int t) {
    // Adds the test to the list of test to run
    // Create the event manager and test controller
    CPPUNIT_NS::TestResult controller;

    // Add a listener that colllects test result
    CPPUNIT_NS::TestResultCollector result;
    controller.addListener(&result);

    // Add a listener that print dots as test run.
    CPPUNIT_NS::BriefTestProgressListener progress;
    controller.addListener(&progress);

    //controller.push

    // Add the top suite to the test runner
    CPPUNIT_NS::TestRunner runner;

//
//  CppUnit::TextUi::TestRunner runner;
//
//  // Change the default outputter to a compiler error format outputter
//  runner.setOutputter(
//      new CppUnit::CompilerOutputter(&runner.result(), std::cerr));

    Test* test = nullptr;

    switch (t) {
        case 1:
            runner.addTest(TestThread::suite());
            break;
        case 2:
            runner.addTest(TestProtoServer::suite());
            break;
        case 101:
            runner.addTest(TestRDMAServer::suite());
            break;
        case 102:
            runner.addTest(TestRDMAServerMultClients::suite());
            break;
        case 103:
            runner.addTest(TestRDMAServerManagementQueue::suite());
            break;
        case 104:
            runner.addTest(TestSimpleUD::suite());
            break;
        case 105:
            runner.addTest(TestRDMAServerMCast::suite());
            break;
        default:
            cout << "No test with number " << t << " exists." << endl;
            return;
    }

    // Run the tests.
    if (test != nullptr) {
        test->test();
        delete test;
    } else {
        runner.run(controller);

        // Print test in a compiler compatible format.
        CPPUNIT_NS::CompilerOutputter outputter(&result, std::cerr);
        outputter.write();

    }
}

static void runserver(int s, int p) {
    ProtoServer* server = nullptr;
    switch (s) {
        default:
            cout << "No server with number " << s << " exists." << endl;
            return;
    }

    if (server->startServer()) {
        while (server->isRunning()) {
            usleep(Config::ISTORE_SLEEP_INTERVAL);
        }
    } else {
        server->stopServer();
    }
    delete server;
}

static void runclient(int s) {
    Thread* client = nullptr;
    switch (s) {
        default:
            cout << "No client with number " << s << " exists." << endl;
            return;
    }

    client->start();
    client->join();
    delete client;
}

struct config_t {
    string runmode = "";
    int number = 0;
    int port = 0;
};

int main(int argc, char* argv[]) {
    struct config_t config;

    while (1) {
        struct option long_options[] = { { "run-mode", required_argument, 0, 'r' }, { "number",
        required_argument, 0, 'n' }, { "port",
        required_argument, 0, 'p' } };

        int c = getopt_long(argc, argv, "r:n:p:", long_options, NULL);
        if (c == -1)
            break;

        switch (c) {
            case 'r':
                config.runmode = string(optarg);
                break;
            case 'n':
                config.number = strtoul(optarg, NULL, 0);
                break;
            case 'p':
                std::cout << "P" << std::endl;
                config.port = strtoul(optarg, NULL, 0);
                std::cout << "Port " << config.port  << std::endl;
                break;
            default:
                usage();
                return 1;
        }
    }

    // load  configuration
    static Config conf;

    // run program
    if (config.runmode.length() > 0) {
        if (config.runmode.compare("test") == 0 && config.number > 0) {
            runtest(config.number);
            return 0;
        } else if (config.runmode.compare("server") == 0 && config.number > 0) {
            runserver(config.number,config.port);
            return 0;
        } else if (config.runmode.compare("client") == 0 && config.number > 0) {
            runclient(config.number);
            return 0;
        }
    }
    usage();
}
