cmake_minimum_required(VERSION 3.10)
project(tcp-cpp VERSION 0.1.0 LANGUAGES CXX)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add your source files
add_executable(tcp-cpp
    server/server.cpp
    server/utils.cpp
)

# Enable testing (optional)
include(CTest)
enable_testing()

# CPack config (optional)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
