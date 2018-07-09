#include "TestNodeClient.h"

void TestNodeClient::setUp()
{

  m_nodeClient = new NodeClient();
  m_regClient = new RegistryClient();
};

void TestNodeClient::tearDown()
{
  delete m_nodeClient;
  delete m_regClient;
};

void TestNodeClient::testAppendShared()
{
  string bufferName = "test";

  BuffHandle *buffHandle = m_regClient->dpi_create_buffer(bufferName, 1, 1024, 1024 * 0.8);
  BufferWriter<BufferWriterShared> buffWriter(buffHandle);
  int data = 1;
  CPPUNIT_ASSERT(m_nodeClient->dpi_append(&buffWriter, (void *)&data, sizeof(int)));
};