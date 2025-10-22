# tcp-cpp
Multi-threaded TCP Client/Server implementation in C++

## Introduction

The purpose of this project is to familiarise with socket programming in modern C++17 syntax,
by utilizing the standard thread/mutex libraries to manage concurrency and synchronization with conditional variables.

TCP is a reliable protocol that guarantees that the data remain intact and arrive in the same order in which they were sent. While, multithreading allows multiple clients to connect and interact with the server in tandem, without significant drop-offs in transmission time.

## Implementation

- The Client connects and requests a specific directory from the Server. Then, it receives each file along with its information, so that it can locally replicate the same folder hierarchy.

- The Server creates two kinds of threads to handle the requests. The communication threads that are responsible for each client and the worker threads that send the respective files.

![Architectural Diagram](https://user-images.githubusercontent.com/73662635/180067338-e6df7da1-c5e4-4f0c-89e2-a787c2d01608.png)

## Compilation and Execution

Ensure that you have [Clang](https://clang.llvm.org/get_started.html) ver. >17.0 installed locally. For MacOS users, see [this StackOverflow post](https://stackoverflow.com/questions/53111082/how-to-install-clang-tidy-on-macos) on how to set up Clang-Tidy and other Clang toolings.

Also ensure that you have VSCode installed with all of the recommended extensions. Then, clone this repository. From the root directory of the project, run the following commands on the terminal:

```bash
# To build the project via CMake
cmake -B build -S .
cmake --build build

# In one terminal, spin up the server instance:
# ./build/server [-p <PORT>] [-s <THREAD POOL SIZE>] [-q <QUEUE SIZE>] [-b <BLOCK SIZE>]
./build/server -p 8080 -s 4 -q 10 -b 1024 # values specified here are by default

# In separate terminal(s), spin up client instance(s):
# ./build/client [-i <SERVER IP>] [-p <SERVER PORT>] [-d <FILE DIRECTORY>]
./build/client/ -i localhost -p 8080 -d . # values specified here are by default
```

To delete all the executable and object files generated:

```bash
make clean
```

To run clang-tidy:

```bash
# clang-tidy [-p build] <source-file>
clang-tidy server/server.cpp
```

## Acknowledgements

This project is largely based off of the work of [this GitHub repository](https://github.com/themisvaltinos/TCP-Client-Server). The code has since been largely formatted for modern C++17 syntax and built using CMake.
