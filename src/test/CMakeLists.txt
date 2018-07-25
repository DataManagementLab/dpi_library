FIND_PACKAGE(CPPUNIT REQUIRED)
enable_testing()

add_subdirectory(net)
add_subdirectory(thread)
add_subdirectory(dpi)
add_subdirectory(examples)

set(TEST_SRC
	Test.h
	Tests.h
)

add_library(test ${TEST_SRC})

add_executable(dpi_tests TestRunner.cc)
 
add_test(NAME dpi_tests COMMAND ${EXECUTABLE_OUTPUT_PATH}/dpi_tests)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure 
DEPENDS dpi_tests)
  
SET_TARGET_PROPERTIES(dpi_tests PROPERTIES
   PREFIX ""
   LINK_FLAGS "-Wl,--no-as-needed"
)
	
target_link_libraries(dpi_tests 
	${CPPUNIT_LIBRARY}
	test_thread
	test_net
	test_dpi
	examples_dpi
	)
	
target_link_libraries(test 
	${CPPUNIT_LIBRARY}
	test_thread
	test_net
	test_dpi
	examples_dpi
	)
	
