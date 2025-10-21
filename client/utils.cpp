#include "utils.hpp"

#include <unistd.h>
#include <cstring>
#include <stdexcept>

#include <string>

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
