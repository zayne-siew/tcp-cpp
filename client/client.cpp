#include "client.hpp"

#include <arpa/inet.h>
#include <netdb.h>

#include <iostream>

#include "fileio.hpp"
#include "utils.hpp"

int main (int argc, char* argv[]) {
    ClientConfig clientConfig;

    // Directory to request from the server
    // TODO: Configure this according to what the server is expected to serve
    std::string directory = ".";

    // Parse command-line args
    int opt;
    while ((opt = getopt (argc, argv, "i:p:d:")) != -1) {
        switch (opt) {
        case 'i': clientConfig.server_ip = optarg; break;
        case 'p': clientConfig.server_port = std::stoi (optarg); break;
        case 'd': directory = optarg; break;
        default:
            throw std::runtime_error ("Usage: ./remoteClient [-i <server_ip>] [-p <server_port>] [-d <directory>]");
        }
    }

    std::cout << "\nClient configuration:" << std::endl;
    std::cout << "  Server IP: " << clientConfig.server_ip << std::endl;
    std::cout << "  Port:      " << clientConfig.server_port << std::endl;

    std::cout << "\nRequest-specific parameters:" << std::endl;
    std::cout << "  Directory: " << directory << "\n" << std::endl;

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

    // Handle server response
    // TODO: Configure response handling according to what the server is expected to send
    accept_files_from_server (sockFD, directory);

    close (sockFD);
    std::cout << "\nConnection terminated successfully.\n";
    return EXIT_SUCCESS;
}
