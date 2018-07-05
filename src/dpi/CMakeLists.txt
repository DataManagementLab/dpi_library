
set(DPI_SRC
    NodeClient.h
    NodeServer.h
    RegistryClient.h
    RegistryServer.h
    BufferWriter.h
    BuffHandle.h
  ) # Adding headers required for portability reasons http://voices.canonical.com/jussi.pakkanen/2013/03/26/a-list-of-common-cmake-antipatterns/
add_library(dpi ${DPI_SRC})
target_include_directories(dpi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dpi 
  net_proto
  net_rdma
  net_message
  )
