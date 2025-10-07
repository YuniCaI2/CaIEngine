cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 23)


function(add_framework_test TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} PRIVATE FrameWork)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()


add_framework_test(LoggerTest)
add_framework_test(ParserTest)
add_framework_test(FrameGraphTest)
add_framework_test(threadPoolTest)
add_framework_test(RenderQueueTest)
