set(BENCHMARK_SRC
  DPIAppendBenchmark.h
  DPIAppendBenchmark.cc
  Benchmarks.h
  BenchmarkRunner.h
  
) # Adding headers required for portability reasons http://voices.canonical.com/jussi.pakkanen/2013/03/26/a-list-of-common-cmake-antipatterns/

add_library(benchmark ${BENCHMARK_SRC})

target_include_directories(benchmark PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  
target_link_libraries(
  benchmark net_rdma dpi thread
)
