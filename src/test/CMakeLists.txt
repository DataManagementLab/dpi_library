FIND_PACKAGE(CPPUNIT REQUIRED)
enable_testing()

add_subdirectory(net)
add_subdirectory(thread)

set(TEST_SRC
	Test.h
	Tests.h
)

add_library(test ${TEST_SRC})

add_executable(istore2_tests TestRunner.cc)
 
add_test(NAME istore2_tests COMMAND ${EXECUTABLE_OUTPUT_PATH}/istore2_tests)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure 
DEPENDS istore2_tests)
  
SET_TARGET_PROPERTIES(istore2_tests PROPERTIES
   PREFIX ""
   LINK_FLAGS "-Wl,--no-as-needed"
)
	
target_link_libraries(istore2_tests 
	${CPPUNIT_LIBRARY}
	test_thread
	test_net
	)
	
target_link_libraries(test 
	${CPPUNIT_LIBRARY}
	test_thread
	test_net
	)
	
