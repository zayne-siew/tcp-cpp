#include "client.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

/**
 * Read exactly `size` bytes from a socket.
 *
 * @param sockFD The socket file descriptor to read from.
 * @param buffer The buffer to read data into.
 * @param size The number of bytes to read.
 * @param context A string describing the context for error messages.
 *
 * @throws std::runtime_error on error or if connection is closed prematurely.
 */
void read_exact (int sockFD, void* buffer, size_t size, const char* context) {
    auto* buf         = static_cast<char*> (buffer);
    size_t total_read = 0;

    while (total_read < size) {
        ssize_t bytes_read = read (sockFD, buf + total_read, size - total_read); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (bytes_read <= 0) {
            throw std::runtime_error (
                std::string (context) + ": " +
                (bytes_read == 0 ? "Connection closed" : std::strerror (errno)));
        }
        total_read += bytes_read;
    }
}

/**
 * Write exactly `size` bytes to a socket.
 *
 * @param sockFD The socket file descriptor to write to.
 * @param buffer The buffer containing data to write.
 * @param size The number of bytes to write.
 * @param context A string describing the context for error messages.
 */
void write_exact (int sockFD, const char* buffer, size_t size, const char* context) {
    size_t total_written = 0;

    while (total_written < size) {
        ssize_t bytes_written =
            write (sockFD, buffer + total_written, size - total_written); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (bytes_written <= 0) {
            throw std::runtime_error (
                std::string (context) + ": " +
                (bytes_written == 0 ? "Connection closed" : std::strerror (errno)));
        }
        total_written += bytes_written;
    }
}

int main (int argc, char* argv[]) {
    ClientConfig clientConfig;

    // Parse command-line args
    int opt;
    while ((opt = getopt (argc, argv, "i:p:d:")) != -1) {
        switch (opt) {
        case 'i': clientConfig.server_ip = optarg; break;
        case 'p': clientConfig.server_port = std::stoi (optarg); break;
        case 'd': clientConfig.directory = optarg; break;
        default:
            throw std::runtime_error ("Usage: ./remoteClient [-i <server_ip>] [-p <server_port>] [-d <directory>]");
        }
    }

    std::cout << "\nClient parameters:" << std::endl;
    std::cout << "  Server IP: " << clientConfig.server_ip << std::endl;
    std::cout << "  Port:      " << clientConfig.server_port << std::endl;
    std::cout << "  Directory: " << clientConfig.directory << "\n" << std::endl;

    // Resolve host
    hostent* server_host = gethostbyname (clientConfig.server_ip.c_str ());
    if (server_host == nullptr) {
        herror ("gethostbyname");
        return EXIT_FAILURE;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr = *reinterpret_cast<struct in_addr*> (server_host->h_addr); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    server_addr.sin_port = htons (clientConfig.server_port);

    // Create socket
    int sockFD = socket (AF_INET, SOCK_STREAM, 0);
    if (sockFD < 0) {
        perror ("Unable to create socket");
        return EXIT_FAILURE;
    }

    // Connect
    if (connect (sockFD, reinterpret_cast<sockaddr*> (&server_addr), sizeof (server_addr)) < 0) {
        perror ("Unable to connect");
        close (sockFD);
        return EXIT_FAILURE;
    }
    std::cout << "Connected to " << inet_ntoa (server_addr.sin_addr) << ":"
              << clientConfig.server_port << "\n"
              << std::endl;

    // Send directory path
    write_exact (sockFD, clientConfig.directory.c_str (), clientConfig.directory.size () + 1, "Error sending directory");

    // Receive block size
    uint32_t net_block_size;
    read_exact (sockFD, &net_block_size, sizeof (uint32_t), "Error reading block size");
    uint32_t block_size = ntohl (net_block_size);

    // Receive file count
    uint32_t net_file_count;
    read_exact (sockFD, &net_file_count, sizeof (uint32_t), "Error reading file count");
    uint32_t file_count = ntohl (net_file_count);

    std::vector<char> buffer (block_size);

    for (uint32_t i = 0; i < file_count; ++i) {
        // Read filename size and name
        uint32_t net_name_size;
        read_exact (sockFD, &net_name_size, sizeof (uint32_t), "Error reading name size");
        uint32_t name_size = ntohl (net_name_size);

        std::string filepath (name_size, '\0');
        read_exact (sockFD, filepath.data (), name_size, "Error reading filepath");

        fs::path file_path (filepath);
        fs::path dir_path = file_path.parent_path ();

        // Ensure directory exists
        std::error_code error_code;
        fs::create_directories (dir_path, error_code);

        // Remove existing file if any
        if (fs::exists (file_path)) {
            fs::remove (file_path, error_code);
        }

        // Open output file
        std::ofstream out (file_path, std::ios::binary);
        if (!out) {
            throw std::runtime_error (
                "Failed to open file for writing: " + file_path.string ());
        }

        // Read file size
        uint32_t net_file_size;
        read_exact (sockFD, &net_file_size, sizeof (uint32_t), "read file size");
        size_t file_size = ntohl (net_file_size);

        // Read and write in chunks
        while (file_size > 0) {
            size_t to_read = std::min<size_t> (file_size, block_size);
            read_exact (sockFD, buffer.data (), to_read, "read file data");
            out.write (buffer.data (), (long)to_read);
            file_size -= to_read;
        }

        out.close ();
        std::cout << "Received: " << file_path << "\n";
    }

    close (sockFD);
    std::cout << "\nAll files received successfully.\n";
    return EXIT_SUCCESS;
}
