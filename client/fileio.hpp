#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * Accept a file from the socket and write it to buffer.
 * @param sockFD The socket file descriptor to read from.
 * @param buffer A pre-allocated buffer for reading file data.
 * @param block_size The size of each read block.
 */
void accept_file (int sockFD, std::vector<char>& buffer, uint32_t block_size);

/**
 * Accept multiple files from the server and write them to the specified directory.
 * @param sockFD The socket file descriptor to read from.
 * @param directory The directory to save the received files.
 */
void accept_files_from_server (int sockFD, const std::string& directory);
