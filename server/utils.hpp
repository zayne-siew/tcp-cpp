#pragma once

#include <string>

#include "server.hpp"

/**
 * Binds a socket to a specified port.
 *
 * @param socketFD File descriptor of the socket to bind.
 * @param port Port number to bind the socket to.
 *
 * @return Returns 0 on success; otherwise, exits the program on error.
 */
int bind_port (int socketFD, std::uint16_t port);

/**
 * Helper function to handle client requests.
 *
 * This function reads the directory path from the client, counts the files,
 * sends the block size and file count to the client, and catalogs the files
 * in the specified directory and its subdirectories.
 *
 * @param connectFD The file descriptor of the client connection.
 */
void get_client_request (int connectFD);

/**
 * Handles a client request by sending the requested file.
 *
 * @param client_request A string containing the client connection file descriptor and the file path.
 *
 * @note This function assumes the format "<connectFD> <file_path>" for client_request.
 */
void handle_client_request (const std::string& client_request);

/**
 * Worker thread function that processes tasks from the work queue.
 *
 * This function continuously waits for tasks to be available in the queue,
 * retrieves them, and processes them by calling `handle_client_request`.
 * The thread exits gracefully when the `stop_flag` is set and the queue is empty.
 *
 * @note This function is designed to be run in a separate thread.
 */
void worker_thread ();
