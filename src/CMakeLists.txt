enable_testing()

add_subdirectory(utils)
add_subdirectory(thread)
add_subdirectory(net)
add_subdirectory(test)


add_executable(dpi Main.cc)

target_link_libraries(dpi
  utils
  net_message
  net_proto
  net_rdma
  test
)

