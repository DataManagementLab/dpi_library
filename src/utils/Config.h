

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

//Includes
#include <iostream>
#include <stddef.h>
#include <sstream>
#include <unistd.h>
#include <stdint.h>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <google/protobuf/stubs/common.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h> /* For strncpy */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

using namespace std;

#define RDMA_TRANSPORT 0 //0=RC, 1=UD

#if RDMA_TRANSPORT == 0
#define DPI_UNIT_TEST_RC(test) CPPUNIT_TEST(test)
#define DPI_UNIT_TEST_UD(test)
#elif RDMA_TRANSPORT == 1
#define DPI_UNIT_TEST_RC(test)
#define DPI_UNIT_TEST_UD(test) CPPUNIT_TEST(test)
#endif

//#define DEBUGCODEANOTHER
#if defined(DEBUGCODEANOTHER)

#define DEBUG_WRITE(outputStream, className, funcName, message)                     \
    do                                                                              \
    {                                                                               \
        std::string header = std::string("[") + className + "::" + funcName + "] "; \
        outputStream << std::left << header << message << std::endl;                \
    } while (false)

#define RESULT_WRITE(outputStream, message)                \
    do                                                     \
    {                                                      \
        outputStream << std::left << message << std::endl; \
    } while (false)

#define DEBUG_OUT(x)                     \
    do                                   \
    {                                    \
        if (debugging_enabled)           \
        {                                \
            std::cout << x << std::endl; \
        }                                \
    } while (0);

#else
#define DEBUG_WRITE(outputStream, className, funcName, message)
#define RESULT_WRITE(outputStream, message)
#define DEBUG_OUT(x)
#endif



#define DEBUGCODE
#if defined(DEBUGCODE)
#define DebugCode(code_fragment) \
    {                            \
        code_fragment            \
    }
#else
#define DebugCode(code_fragment)
#endif


#define DPI_DEBUG_LEVEL 1

#ifdef DPI_DEBUG_LEVEL
#define DPI_DEBUG(X, ...) if(DPI_DEBUG_LEVEL>0) {fprintf(stdout, X, ##__VA_ARGS__); fflush(stdout);}
#else
#define DPI_DEBUG(X, ...) {}
#endif

//To be implemented MACRO
#define TO_BE_IMPLEMENTED(code_fragment)
#define DPI_UNIT_TEST_SUITE(suite) CPPUNIT_TEST_SUITE(suite)
#define DPI_UNIT_TEST(test) CPPUNIT_TEST(test)
#define DPI_UNIT_TEST_SUITE_END() CPPUNIT_TEST_SUITE_END()

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

//typedefs
typedef unsigned long long uint128_t;
typedef uint64_t NodeID;
typedef uint64_t Offset;

namespace dpi
{

//Constants
class Config
{
  public:
    Config()
    {
        load();
    }

    ~Config()
    {
        unload();
    }

    //DPI
    const static int DPI_MAX_SOCKETS = 1024;
    const static int DPI_SLEEP_INTERVAL = 100 * 1000;
    constexpr static double DPI_SEGMENT_THRESHOLD_FACTOR = 0.8;
    
    static string DPI_REGISTRY_SERVER;
    static uint32_t DPI_REGISTRY_PORT;
    static vector<string> DPI_NODES;

    static uint32_t DPI_NODE_PORT;
    static uint32_t DPI_INTERNAL_BUFFER_SIZE;
    static uint32_t DPI_SEGMENT_SIZE;

    /**
     * @brief DPI_SEGMENT_HEADER_t describes the header of a segmnet
     * NOTE: if modified please adapt the DPI_SEGMENT_HEADER_META and update 
     *       the offsets accordingly
     */
    struct DPI_SEGMENT_HEADER_t
    {
        uint64_t counter = 0;
        uint64_t hasFollowSegment = 0;
    };
    struct DPI_SEGMENT_HEADER_META
    {
        static const size_t getCounterOffset = 0;
        static const size_t getHasFollowSegmentOffset = sizeof(DPI_SEGMENT_HEADER_t::counter);
    };

    //RDMA
    static size_t RDMA_MEMSIZE;
    static uint32_t RDMA_PORT;
    static uint32_t RDMA_NUMAREGION;
    static uint32_t RDMA_DEVICE;
    static uint32_t RDMA_IBPORT;
    static uint32_t RDMA_MAX_WR;
    const static uint32_t RDMA_MAX_SGE = 1;
    const static size_t RDMA_UD_OFFSET = 40;
    
    //THREAD
    static vector<int> THREAD_CPUS;

    //LOGGING
    static int LOGGING_LEVEL; //0=all, 1=ERR, 2=DBG, 3=INF, (>=4)=NONE

    //TEST
    static int HELLO_PORT;

    static string& getIPFromNodeId(NodeID& nodeid);

  private:
    static void load();
    static void unload();

    static void set(string key, string value);
    static void init_vector(vector<string> &values, string csv_list);
    static void init_vector(vector<int> &values, string csv_list);

    static string getIP();
};

} // end namespace dpi

using namespace dpi;

#endif /* CONFIG_HPP_ */
