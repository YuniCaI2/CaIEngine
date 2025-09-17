cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 23)


add_executable(loggerTest LoggerTest.cpp)
target_link_libraries(loggerTest FrameWork)
add_test(NAME loggerTest COMMAND loggerTest)

add_executable(parserTest parserTest.cpp)
target_link_libraries(parserTest FrameWork)
add_test(NAME parserTest COMMAND parserTest)

add_executable(frameGraphTest frameGraphTest.cpp)
target_link_libraries(frameGraphTest FrameWork)
add_test(NAME frameGraphTest COMMAND frameGraphTest)

add_executable(threadPoolTest threadPoolTest.cpp)
target_link_libraries(threadPoolTest FrameWork)
add_test(NAME threadPoolTest COMMAND threadPoolTest)