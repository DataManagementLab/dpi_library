set(EXAMPLES_DPI_SRC

append_examples.h
append_examples.cc
  ) # Adding headers required for portability reasons http://voices.canonical.com/jussi.pakkanen/2013/03/26/a-list-of-common-cmake-antipatterns/
  
add_library(examples_dpi ${EXAMPLES_DPI_SRC})

target_include_directories(examples_dpi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(examples_dpi net_rdma dpi)
