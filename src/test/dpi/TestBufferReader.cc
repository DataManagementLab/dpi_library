#include "TestBufferReader.h"
#include "../../net/rdma/RDMAClient.h"

void TestBufferReader::setUp()
{
    //Setup Test DPI
    Config::RDMA_MEMSIZE = 1024ul * 1024 * 1024 * 1; //1GB
    Config::DPI_SEGMENT_SIZE = (2048 + sizeof(Config::DPI_SEGMENT_HEADER_t));
    Config::DPI_INTERNAL_BUFFER_SIZE = 1024;
    Config::DPI_REGISTRY_SERVER = "127.0.0.1";
    Config::DPI_NODES.clear();
    string dpiTestNode = "127.0.0.1:" + to_string(Config::DPI_NODE_PORT);
    Config::DPI_NODES.push_back(dpiTestNode);

    m_stub_regClient = new RegistryClientStub(new RDMAClient());
    m_nodeServer = new NodeServer();
    CPPUNIT_ASSERT(m_nodeServer->startServer());
};

void TestBufferReader::tearDown()
{
    delete m_stub_regClient;
    if (m_nodeServer->running())
    { 
        m_nodeServer->stopServer();
    }
    delete m_nodeServer;
};

void TestBufferReader::testReadWithHeader()
{
    //ARRANGE
    string bufferName = "buffer1";
    std::cout << "testReadWithHeader" << '\n';
    size_t remoteOffset = 0;
    size_t memSize = sizeof(int);

    uint32_t numberSegments = 2;
    size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / memSize * numberSegments;

    m_stub_regClient->registerBuffer(new BufferHandle(bufferName, 1, numberSegments));
    BufferHandle *buffHandle = m_stub_regClient->createSegmentRingOnBuffer(bufferName);
    
    BufferWriter buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);

    BufferReader bufferReader(buffHandle, m_stub_regClient);
    //ACT
    for (size_t i = 0; i < numberElements; i++)
    {
        CPPUNIT_ASSERT(buffWriter.append((void *)&i, memSize));
    }

    CPPUNIT_ASSERT(buffWriter.close());
    int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);
    size_t read_buffer_size = 0;
    int *read_buffer = (int *)bufferReader.read(read_buffer_size, true);

    //Assert that the read size matches two full segments
    CPPUNIT_ASSERT_EQUAL((size_t)(numberSegments * Config::DPI_SEGMENT_SIZE), read_buffer_size);

    //ASSERT
    for (size_t i = 0; i < numberSegments * Config::DPI_SEGMENT_SIZE / memSize; i++)
    {
        CPPUNIT_ASSERT_EQUAL(rdma_buffer[i], read_buffer[i]);
    }
    m_nodeServer->localFree(rdma_buffer);
    m_nodeServer->localFree(read_buffer);
};

void TestBufferReader::testReadWithoutHeader(){
    //ARRANGE
    string bufferName = "buffer1";
    std::cout << "testReadWithoutHeader" << '\n';

    size_t remoteOffset = 0;
    size_t memSize = sizeof(int);

    uint32_t numberSegments = 2;
    size_t numberElements = (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / memSize * numberSegments;

    m_stub_regClient->registerBuffer(new BufferHandle(bufferName, 1, numberSegments));
    BufferHandle *buffHandle = m_stub_regClient->createSegmentRingOnBuffer(bufferName);

    BufferWriter buffWriter(buffHandle, Config::DPI_INTERNAL_BUFFER_SIZE, m_stub_regClient);

    BufferReader bufferReader(buffHandle, m_stub_regClient);
    //ACT
    for (size_t i = 0; i < numberElements; i++)
    {
        CPPUNIT_ASSERT(buffWriter.append((void *)&i, memSize));
    }

    CPPUNIT_ASSERT(buffWriter.close());
    int *rdma_buffer = (int *)m_nodeServer->getBuffer(remoteOffset);
    size_t read_buffer_size = 0;
    int *read_buffer = (int *)bufferReader.read(read_buffer_size);

    //Assert that the read size matches two full segments minus headers
    CPPUNIT_ASSERT_EQUAL((size_t)(numberSegments * (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t))), read_buffer_size);

    //ASSERT
    for (size_t i = 0; i < numberSegments * (Config::DPI_SEGMENT_SIZE - sizeof(Config::DPI_SEGMENT_HEADER_t)) / memSize; i++)
    {
        CPPUNIT_ASSERT_EQUAL((int)i, read_buffer[i]);
    }
    m_nodeServer->localFree(rdma_buffer);
    m_nodeServer->localFree(read_buffer); 
};
