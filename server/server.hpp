#pragma once

#include <mutex>
#include <unordered_map>

constexpr int DEFAULT_PORT             = 8080;
constexpr int DEFAULT_BLOCK_SIZE       = 1024;
constexpr int DEFAULT_THREAD_POOL_SIZE = 4;
constexpr int DEFAULT_QUEUE_SIZE       = 10;

// Map socket and mutex
extern std::unordered_map<int, std::mutex> socket_mutex_map; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

// Server configuration structure
struct ServerConfig {
    // Port number
    int port = DEFAULT_PORT;
    // Block size for file transfer
    int block_size = DEFAULT_BLOCK_SIZE;
    // Thread pool size
    int thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
    // Queue size
    int queue_size = DEFAULT_QUEUE_SIZE;
};
// Global server configuration
extern ServerConfig serverConfig; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
