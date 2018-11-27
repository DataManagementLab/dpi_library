enable_testing()

add_subdirectory(utils)
add_subdirectory(thread)
add_subdirectory(net)
add_subdirectory(test)
add_subdirectory(dpi)
add_subdirectory(benchmark)
add_subdirectory(api)


add_executable(dpi_exe Main.cc)

target_link_libraries(dpi_exe
  utils
  net_message
  net_proto
  net_rdma
  test
  dpi
  api
)

add_executable(dpi_bench BenchmarkMain.cc)
    
target_link_libraries(dpi_bench
  benchmark
  utils
  dpi
  net_proto
  net_rdma
)