set(API_SRC
    dpi.h
    append.h
    context.h
    create_buffer.h
    err_codes.h
    init.h
    finalize.h
  ) # Adding headers required for portability reasons http://voices.canonical.com/jussi.pakkanen/2013/03/26/a-list-of-common-cmake-antipatterns/
add_library(api ${API_SRC})
target_include_directories(api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(api
  dpi 
  net_rdma
  )