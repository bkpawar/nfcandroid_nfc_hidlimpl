add_subdirectory(googletest)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

set(SOURCES hello_world_test.cpp)
add_executable(runTests ${SOURCES})
target_link_libraries(runTests gtest gtest_main)
add_test(NAME HelloWorldTest COMMAND runTests)