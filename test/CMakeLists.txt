cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 23)


add_executable(loggerTest LoggerTest.cpp)
target_link_libraries(loggerTest FrameWork)
add_test(NAME loggerTest COMMAND loggerTest)
