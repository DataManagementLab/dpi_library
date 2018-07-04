enable_testing()

add_subdirectory(utils)
add_subdirectory(thread)
add_subdirectory(net)
add_subdirectory(test)


add_executable(istore2 Main.cc)

target_link_libraries(istore2
  utils
  net_message
  net_proto
  net_rdma
  test
)

