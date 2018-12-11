set(TEST_DPI_SRC

TestRegistryClient.h
TestRegistryClient.cc
TestBufferWriter.h
TestBufferWriter.cc
IntegrationTestsAppend.h
IntegrationTestsAppend.cc
TestBufferReader.h
TestBufferReader.cc
RegistryClientStub.h
TestBufferConsumer.h
TestBufferConsumer.cc
  ) # Adding headers required for portability reasons http://voices.canonical.com/jussi.pakkanen/2013/03/26/a-list-of-common-cmake-antipatterns/
  
add_library(test_dpi ${TEST_DPI_SRC})

target_include_directories(test_dpi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_dpi net_rdma dpi)
