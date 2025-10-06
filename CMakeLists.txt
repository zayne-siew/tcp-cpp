cmake_minimum_required(VERSION 3.10.0)
project(tcp-cpp VERSION 0.1.0 LANGUAGES CXX)

# add_executable(tcp-cpp main.cpp)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
