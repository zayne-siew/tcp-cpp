#include "utils.hpp"

#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

namespace fs = std::filesystem;

// Buffer size for reading from socket
constexpr std::size_t BUFFER_SIZE = 1024;

std::atomic<bool> stop_flag{ false }; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::queue<std::string> clients; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::unordered_map<int, int> unsatisfied_clients; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

std::mutex uc_mutex; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::mutex pool_mutex; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::condition_variable pool_cond; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::condition_variable full_pool; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

/**
 * Recursively scans the directory and its subdirectories to find files and add them to the work queue.
 *
 * @param dirpath The path of the directory to scan.
 * @param connectFD The file descriptor of the client connection.
 */
void catalog (const std::string& dirpath, int connectFD);

/**
 * Counts the number of regular files in the specified directory and its subdirectories.
 *
 * This function updates the `unsatisfied_clients` map with the count of files
 * for the given client connection file descriptor.
 *
 * @param dirpath The path of the directory to scan.
 * @param connectFD The file descriptor of the client connection.
 */
void count_files (const std::string& dirpath, int connectFD);

int bind_port (int socketFD, std::uint16_t port) {
    // Create and initialize server address
    sockaddr_in server{};
    server.sin_family      = AF_INET;            // IPv4
    server.sin_addr.s_addr = htonl (INADDR_ANY); // Listen on any IP
    server.sin_port        = htons (port);       // TCP port

    // Bind socket
    int result = bind (socketFD, reinterpret_cast<sockaddr*> (&server), sizeof (server));
    if (result < 0) {
        throw std::runtime_error ("Failed to bind socket on port " + std::to_string (port));
    }
    return result;
}

void catalog (const std::string& dirpath, int connectFD) {
    try {
        for (const auto& entry : fs::directory_iterator (dirpath)) {
            const auto& path = entry.path ();
            if (entry.is_regular_file ()) {
                std::string client_and_file =
                    std::to_string (connectFD) + " " + path.string ();

                // Add to the queue safely
                std::unique_lock<std::mutex> lock (pool_mutex);
                // Wait if queue is full
                full_pool.wait (lock, [] () {
                    return clients.size () < serverConfig.queue_size;
                });

                std::cout
                    << "[Thread: "
                    << std::hash<std::thread::id>{}(std::this_thread::get_id ())
                    << "]: Adding file " << path << " to the queue..." << std::endl;

                clients.push (client_and_file);

                // Notify worker threads
                pool_cond.notify_one ();
            } else if (entry.is_directory ()) {
                // Recursive call for subdirectories
                catalog (path.string (), connectFD);
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error accessing directory " << dirpath << ": "
                  << e.what () << std::endl;
    }
}

// void get_client_request (int connectFD) {
//     std::string dirpath;
//     std::vector<char> buffer (BUFFER_SIZE);

//     // Read folder path from client
//     ssize_t bytes_read;
//     while ((bytes_read = read (connectFD, buffer.data (), buffer.size ())) > 0) {
//         dirpath.append (buffer.data (), static_cast<size_t> (bytes_read));
//         if (bytes_read < static_cast<ssize_t> (buffer.size ())) {
//             break;
//         }
//     }

//     // Read error handling
//     if (bytes_read < 0) {
//         perror ("Failed to read directory path from client socket");
//         return;
//     }

//     std::cout
//         << "[Thread: " << std::hash<std::thread::id>{}(std::this_thread::get_id ())
//         << "]: About to scan directory " << dirpath << std::endl;

//     // How many files need to be processed
//     count_files (dirpath, connectFD);

//     // Send server block size to client
//     uint32_t server_block_size_network = htonl (serverConfig.block_size);
//     if (write (connectFD, &server_block_size_network, sizeof (server_block_size_network)) < 0) {
//         perror ("Failed to send server block size to client");
//         return;
//     }

//     // Send file count to client
//     int counter            = unsatisfied_clients[connectFD];
//     uint32_t count_network = htonl (static_cast<uint32_t> (counter));
//     if (write (connectFD, &count_network, sizeof (count_network)) < 0) {
//         perror ("Failed to send file count to client");
//         return;
//     }

//     // Recursively find and add files in the queue
//     catalog (dirpath, connectFD);
// }

void get_client_request (int connectFD) {
    std::string data;
    std::vector<char> buffer (BUFFER_SIZE);

    // Read data from client
    ssize_t bytes_read;
    while ((bytes_read = read (connectFD, buffer.data (), buffer.size ())) > 0) {
        data.append (buffer.data (), static_cast<size_t> (bytes_read));
        if (bytes_read < static_cast<ssize_t> (buffer.size ())) {
            break;
        }
    }

    // Read error handling
    if (bytes_read < 0) {
        perror ("Failed to read data from client socket");
        return;
    }

    std::cout
        << "[Thread: " << std::hash<std::thread::id>{}(std::this_thread::get_id ())
        << "]: " << data << std::endl;
}

void count_files (const std::string& dirpath, int connectFD) {
    try {
        // Check that the path exists and is a directory
        if (!fs::exists (dirpath) || !fs::is_directory (dirpath)) {
            throw std::runtime_error ("Invalid directory: " + dirpath);
        }

        // Recursively iterate through the directory
        for (const auto& entry : fs::recursive_directory_iterator (dirpath)) {
            if (entry.is_regular_file ()) {
                std::lock_guard<std::mutex> lock (uc_mutex);
                unsatisfied_clients[connectFD]++;
            }
        }

        std::cout
            << "[Thread: " << std::hash<std::thread::id>{}(std::this_thread::get_id ())
            << "]: Counted files in " << dirpath
            << ", total: " << unsatisfied_clients[connectFD] << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error in count_files: " << e.what () << std::endl;
    }
}

void worker_thread () {
    while (!stop_flag.load ()) {
        std::string client_request;

        {
            // Acquire lock for accessing the queue
            std::unique_lock<std::mutex> lock (pool_mutex);
            // Wait until there is work to do
            pool_cond.wait (lock, [] {
                return !clients.empty () || stop_flag.load ();
            });

            if (stop_flag.load () && clients.empty ()) {
                return; // Graceful exit
            }

            // Get the next task
            client_request = std::move (clients.front ());
            clients.pop ();

            std::cout << "[Thread: "
                      << std::hash<std::thread::id>{}(std::this_thread::get_id ())
                      << "]: Received task: <" << client_request << ">" << std::endl;

            // Notify producers waiting for space
            full_pool.notify_one ();
        }

        // Process the task outside the lock
        handle_client_request (client_request);
    }
}

void handle_client_request (const std::string& client_request) {
    // ----------------------------
    // 1. Parse client_request
    // ----------------------------
    auto space_pos = client_request.find (' ');
    if (space_pos == std::string::npos) {
        std::cerr << "Invalid client request format: " << client_request << std::endl;
        return;
    }

    int connectFD         = std::stoi (client_request.substr (0, space_pos));
    std::string file_path = client_request.substr (space_pos + 1);

    // ----------------------------
    // 2. Lock the per-socket mutex
    // ----------------------------
    std::mutex* socket_mutex_ptr = nullptr;
    {
        std::lock_guard<std::mutex> lock (uc_mutex);
        auto iterator = socket_mutex_map.find (connectFD);
        if (iterator == socket_mutex_map.end ()) {
            std::cerr << "Mutex for socket " << connectFD << " unavailable" << std::endl;
            return;
        }
        socket_mutex_ptr = &iterator->second;
    }

    std::lock_guard<std::mutex> socket_lock (*socket_mutex_ptr);

    std::cout
        << "[Thread: " << std::hash<std::thread::id>{}(std::this_thread::get_id ())
        << "]: Sending file " << file_path << " to client" << std::endl;

    // ----------------------------
    // 3. Open file and get size
    // ----------------------------
    std::ifstream file (file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open ()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }

    // Get file size
    std::streamsize file_size = file.tellg ();
    file.seekg (0, std::ios::beg);

    // ----------------------------
    // 4. Send metadata
    // ----------------------------
    uint32_t name_size_net = htonl (static_cast<uint32_t> (file_path.size ()));
    if (write (connectFD, &name_size_net, sizeof (name_size_net)) < 0) {
        perror ("Failed to send filename length to client");
        return;
    }

    if (write (connectFD, file_path.c_str (), file_path.size ()) < 0) {
        perror ("Failed to send filename to client");
        return;
    }

    uint32_t file_size_net = htonl (static_cast<uint32_t> (file_size));
    if (write (connectFD, &file_size_net, sizeof (file_size_net)) < 0) {
        perror ("Failed to send file size to client");
        return;
    }

    // ----------------------------
    // 5. Send file in blocks
    // ----------------------------
    std::vector<char> buffer (serverConfig.block_size);
    while (file) {
        file.read (buffer.data (), (long)buffer.size ());
        std::streamsize bytes_read = file.gcount ();

        ssize_t remaining = bytes_read;
        auto iterator     = buffer.begin ();

        while (remaining > 0) {
            ssize_t bytes_written = write (connectFD, &(*iterator), remaining); // take address of iterator element
            if (bytes_written < 0) {
                perror ("Failed to send file data block to client");
                return;
            }
            remaining -= bytes_written;
            std::advance (iterator, bytes_written); // move iterator forward
        }
    }

    file.close ();

    // ----------------------------
    // 6. Update unsatisfied_clients
    // ----------------------------
    {
        std::lock_guard<std::mutex> lock (uc_mutex);
        auto iterator = unsatisfied_clients.find (connectFD);
        if (iterator == unsatisfied_clients.end ()) {
            std::cerr << "unsatisfied_clients key missing for " << connectFD << std::endl;
        } else if (iterator->second > 1) {
            iterator->second -= 1;
        } else {
            unsatisfied_clients.erase (iterator);
            close (connectFD); // Close socket if last file sent
        }
    }
}
