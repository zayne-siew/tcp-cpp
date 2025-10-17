#pragma once

#include <string>

constexpr int DEFAULT_SERVER_PORT = 8080;

// Client configuration structure
struct ClientConfig {
    // Server IP address
    std::string server_ip = "localhost";
    // Server port number
    int server_port = DEFAULT_SERVER_PORT;
};
