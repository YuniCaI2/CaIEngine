cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 23)


function(add_framework_test TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} PRIVATE FrameWork)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

file(GLOB_RECURSE ALL_TEST_SOURCES CONFIGURE_DEPENDS "*Test.cpp")

foreach(SOURCE_FILE ${ALL_TEST_SOURCES})
    get_filename_component(TEST_NAME ${SOURCE_FILE} NAME_WE)
    add_framework_test(${TEST_NAME})
endforeach()