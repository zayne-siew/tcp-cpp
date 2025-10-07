#pragma once

#include <string>

// Client configuration structure
struct ClientConfig {
    // Server IP address
    std::string server_ip = "localhost";
    // Server port number
    int server_port = 8080;
    // Directory to request
    std::string directory = ".";
};
