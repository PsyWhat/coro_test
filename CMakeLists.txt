cmake_minimum_required(VERSION 3.0.0)
project(coroutines_test VERSION 0.1.0 LANGUAGES C CXX)

#set(CMAKE_DEBUG_POSTFIX d)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(coroutines_test src/main.cpp)
#set_target_properties(coroutines_test PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

