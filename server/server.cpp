#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "utils.h"

// Maximum number of pending connections
constexpr std::size_t BACKLOG = 1280;

ServerConfig serverConfig; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::unordered_map<int, std::mutex> socket_mutex_map;

/**
 * Main function to start the server.
 *
 * The server listens for incoming connections and spawns worker threads
 * to handle client requests concurrently. It uses a thread pool and a
 * work queue to manage tasks efficiently.
 */
int main (int argc, char** argv) {
    std::mutex sm_mutex;

    try {
        // Parse command-line args
        int opt;
        while ((opt = getopt (argc, argv, "p:s:q:b:")) != -1) {
            switch (opt) {
            case 'p': serverConfig.port = std::stoi (optarg); break;
            case 's': serverConfig.thread_pool_size = std::stoi (optarg); break;
            case 'q': serverConfig.queue_size = std::stoi (optarg); break;
            case 'b': serverConfig.block_size = std::stoi (optarg); break;
            default:
                throw std::runtime_error (
                    "Usage: ./dataServer [-p port] [-s thread_pool_size] "
                    "[-q queue_size] [-b block_size]");
            }
        }

        std::cout << "\nServer parameters:" << std::endl;
        std::cout << "PORT = " << serverConfig.port << std::endl;
        std::cout << "thread_pool_size = " << serverConfig.thread_pool_size << std::endl;
        std::cout << "queue_size = " << serverConfig.queue_size << std::endl;
        std::cout << "block_size = " << serverConfig.block_size << std::endl;

        // Worker thread pool
        std::vector<std::thread> thread_pool;
        thread_pool.reserve (serverConfig.thread_pool_size);
        for (int i = 0; i < serverConfig.thread_pool_size; ++i) {
            thread_pool.emplace_back (worker_thread);
        }

        // Create and bind socket
        int socketFD = socket (AF_INET, SOCK_STREAM, 0);
        if (socketFD < 0) {
            throw std::runtime_error (
                "Failed to create socket: " + std::string (strerror (errno)));
        }

        bind_port (socketFD, serverConfig.port);
        std::cout << "Server initialized..." << std::endl;

        if (listen (socketFD, BACKLOG) < 0) {
            throw std::runtime_error ("Listen failed: " + std::string (strerror (errno)));
        }
        std::cout << "Listening for connections on port " << serverConfig.port << std::endl;

        // Listen for clients to accept
        while (true) {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof (client_addr);

            int connectFD =
                accept (socketFD, reinterpret_cast<sockaddr*> (&client_addr), &addr_len);
            if (connectFD < 0) {
                throw std::runtime_error ("Accept failed: " + std::string (strerror (errno)));
            }

            std::cout << "Accepted connection from "
                      << inet_ntoa (client_addr.sin_addr) << std::endl;

            // Ensure mutex exists for this socket
            {
                std::lock_guard<std::mutex> lock (sm_mutex);
                socket_mutex_map.try_emplace (connectFD);
            }

            enqueue_client (connectFD);
        }

        // Wait for worker threads to finish
        for (auto& thread : thread_pool) {
            if (thread.joinable ()) {
                thread.join ();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what () << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
