#pragma once

#include <unistd.h>

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
void read_exact (int sockFD, void* buffer, size_t size, const char* context);

/**
 * Write exactly `size` bytes to a socket.
 *
 * @param sockFD The socket file descriptor to write to.
 * @param buffer The buffer containing data to write.
 * @param size The number of bytes to write.
 * @param context A string describing the context for error messages.
 */
void write_exact (int sockFD, const char* buffer, size_t size, const char* context);
