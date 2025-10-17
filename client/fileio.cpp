#include "fileio.hpp"

#include <arpa/inet.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "utils.hpp"

namespace fs = std::filesystem;

void accept_file (int sockFD, std::vector<char>& buffer, uint32_t block_size) {
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

void accept_files_from_server (int sockFD, const std::string& directory) {
    // Send directory path
    write_exact (sockFD, directory.c_str (), directory.size () + 1, "Error sending directory");

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
        accept_file (sockFD, buffer, block_size);
    }
}
