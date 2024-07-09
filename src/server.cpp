#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char **argv) {

  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }

  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    close(server_fd);
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    close(server_fd);
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    close(server_fd);
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  // Accept 200 OK
  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);

  if(client_fd < 0) {
    std::cerr << "error occurred in client connection";
    close(server_fd);
    return 1;
  }

  std::cout << "Client connected\n";


  // Extract URL Path and Respond with 404
  std::string client_req(1024, '\0');

  ssize_t recv_req = recv(client_fd, (void *)&client_req[0], client_req.max_size(), 0);

  if(recv_req < 0) {
    std::cerr << "error occurred in receiving request";
    close(server_fd);
    return 1;
  }

  std::cerr << "Client Message (length: " << client_req.size() << ")" << std::endl;
  std::clog << client_req << std::endl;

  std::string response = client_req.starts_with("GET / HTTP/1.1\r\n") ? "HTTP/1.1 200 OK\r\n\r\n" : "HTTP/1.1 404 Not Found\r\n\r\n";

  ssize_t sent_req = send(client_fd, response.c_str(), response.size(),0);

  if(sent_req < 0) {
   std::cerr << "error occured in sending request to the server";
    close(client_fd);
    close(server_fd);
    return 1;
  }

  close(client_fd);
  close(server_fd);
  return 0;
}
