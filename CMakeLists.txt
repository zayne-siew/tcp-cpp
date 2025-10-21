cmake_minimum_required(VERSION 3.10)
project(tcp-cpp VERSION 0.1.0 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
# Server executable
# -----------------------------
add_executable(server
    server/server.cpp
    server/utils.cpp
)

# -----------------------------
# Client executable
# -----------------------------
add_executable(client
    client/client.cpp
    client/fileio.cpp
    client/utils.cpp
)

if (WIN32)
    target_link_libraries(server PRIVATE ws2_32)
    target_link_libraries(client PRIVATE ws2_32)
endif()

# Enable testing (optional)
include(CTest)
enable_testing()

# CPack config (optional)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
