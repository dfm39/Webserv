#include <iostream>
#include <fstream>
#include "./includes/Configuration.hpp"

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>

#define MAX_CLIENTS 10
#define BUF_SIZE 4096
#define LISTENING_SOCKETS_NUMBER 2
#define ETC_PORT 4003

#include <fstream>
#include <string>


// static void handle_root(int connfd) {
//     char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Welcome to the root page</h1>";
//     write(connfd, response, strlen(response));
// }

// static void handle_page1(int connfd) {
//     char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Welcome to page 1</h1>";
//     write(connfd, response, strlen(response));
// }

// static void handle_page2(int connfd) {
//     char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Welcome to page 2</h1>";
//     write(connfd, response, strlen(response));
// }

// static void handle_not_found(int connfd) {
//     char* response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 Not Found</h1>";
//     write(connfd, response, strlen(response));
// }

static std::string read_file(const std::string& filename) {
    std::ifstream file(filename.c_str());

    if (!file) {
        std::cerr << "Error opening file " << filename << std::endl;
        return "";
    }

    std::string content;
    char c;

    while (file.get(c)) {
        content += c;
    }

    file.close();

    return content;
}

static int handleRequest(int port) {

    // Create a socket for the server
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    //set the socket to non blocking
    // fcntl(server_fd, F_SETFL, O_NONBLOCK);

    // Set the server socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Error setting server socket options");
        exit(EXIT_FAILURE);
    }


    // Bind the server socket to a specific port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding server socket port");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections on the server socket
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Error listening on server socket");
        exit(EXIT_FAILURE);
    }

    // int etc_fd = create_socket(etc_port, &server_addr);

   int etc_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (etc_fd < 0) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    //set the socket to non blocking
    fcntl(etc_fd, F_SETFL, O_NONBLOCK);

    // Set the server socket options
    int etc_opt = 2;
    if (setsockopt(etc_fd, SOL_SOCKET, SO_REUSEADDR, &etc_opt, sizeof(etc_opt))) {
        perror("Error setting server socket options");
        exit(EXIT_FAILURE);
    }

    // Bind the server socket to a specific port
    struct sockaddr_in server_addr_etc;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr_etc.sin_family = AF_INET;
    server_addr_etc.sin_addr.s_addr = INADDR_ANY;
    server_addr_etc.sin_port = htons(ETC_PORT);
    if (bind(etc_fd, (struct sockaddr *)&server_addr_etc, sizeof(server_addr_etc)) < 0) {
        perror("Error binding server socket etc");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections on the server socket
    if (listen(etc_fd, MAX_CLIENTS) < 0) {
        perror("Error listening on server socket");
        exit(EXIT_FAILURE);
    }

    // Initialize the pollfd struct for the server socket
    struct pollfd fds[MAX_CLIENTS];
    memset(fds, 0, sizeof(fds));
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    fds[1].fd = etc_fd;
    fds[1].events = POLLIN;

    // Initialize the list of connected client sockets
    std::vector<int> clients;

    // Start the main loop
    while (true) {

        // Wait for events on any of the sockets
        int nfds = clients.size() + LISTENING_SOCKETS_NUMBER;
        int res = poll(fds, nfds, 200);
        if (res < 0) {
            perror("Error polling sockets");
            exit(EXIT_FAILURE);
        }
        // Check for events on the server socket
        if (fds[0].revents & POLLIN) {

            // Accept a new connection from a client
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd < 0) {
                perror("Error accepting client connection on addr");
                exit(EXIT_FAILURE);
            }

            // Add the client socket to the list of connected clients
            clients.push_back(client_fd);

            // Initialize the pollfd struct for the client socket
            fds[clients.size() + LISTENING_SOCKETS_NUMBER - 1].fd = client_fd;
            fds[clients.size() + LISTENING_SOCKETS_NUMBER - 1].events = POLLOUT;
            fds[clients.size() + LISTENING_SOCKETS_NUMBER - 1].revents = 0;

            std::cout << "New client connected on server fd\n";
        }
        if (fds[1].revents & POLLIN)
        {
            // Accept a new connection from a client
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(etc_fd, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd < 0)
            {
                perror("Error accepting client connection on etc");
                exit(EXIT_FAILURE);
            }

            // Add the client socket to the list of connected clients
            clients.push_back(client_fd);

            // Initialize the pollfd struct for the client socket
            fds[clients.size() + LISTENING_SOCKETS_NUMBER - 1].fd = client_fd;
            fds[clients.size() + LISTENING_SOCKETS_NUMBER - 1].events = POLLIN | POLLOUT;
            fds[clients.size() + LISTENING_SOCKETS_NUMBER - 1].revents = 0;

            std::cout << "New client connected on etc fd\n";
        }
        // Check for events on any of the connected client sockets
        for (unsigned long i = 0; i < clients.size(); i++) 
        {
            // printf("i equals %lu\n",i);
            if (fds[i + LISTENING_SOCKETS_NUMBER].revents & POLLIN) 
            {
                // Receive data from the client
                char buf[BUF_SIZE];
                memset(buf, 0, BUF_SIZE);
                int n = recv(clients[i], buf, BUF_SIZE, 0);
                if (n < 0)
                {
                    perror("Error receiving data from client");
                    exit(EXIT_FAILURE);
                }
                  std::cout << "Received message from client: " << buf << std::endl;
            }
            if (fds[i + LISTENING_SOCKETS_NUMBER].revents & POLLOUT)
            {
            //Send data to client
                std::string http_response = "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: 159\n\n";
                std::string filename = "prototype.html";
                std::string content = read_file(filename);
                std::string message = http_response + content;
                int s = send(clients[i], message.c_str(), message.length(), 0);
                if (s < 0)
                {
                    perror("Error sending data to client");
                    exit(EXIT_FAILURE);
                }



                sleep(1); // this is temporary
            }

        }
        //TODO add usleep here for performance?
        //TODO closing client sockets that disconected?
    }

    // Close all connected client sockets
    for (unsigned long i = 0; i < clients.size(); i++) {
        close(clients[i]);
    }

    // Close the server socket
    close(server_fd);

    return 0;
}

int main (int argc, char **argv)
{
  if (argc != 2)
  {
      std::cerr << "Error: invalid number of arguments, please enter one argument\n";
      return 1;
  }
  Configuration conf;
  if (conf.parseSetListen(argv[1], "listen"))
    return 2;
  std::cout << conf.listen << "\n";
  handleRequest(conf.listen);
}