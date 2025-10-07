#pragma once

#include <mutex>
#include <unordered_map>

// Map socket and mutex
extern std::unordered_map<int, std::mutex> socket_mutex_map;

// Server configuration structure
struct ServerConfig {
    // Port number
    int port = 8080;
    // Block size for file transfer
    int block_size = 1024;
    // Thread pool size
    int thread_pool_size = 4;
    // Queue size
    int queue_size = 10;
};
// Global server configuration
extern ServerConfig serverConfig;
