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
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  // Respond with 200
  int client = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);

  std::string message = "HTTP/1.1 200 OK\r\n\r\n";

  int sendMsg = send(client, message.c_str(), message.length(), 0);

  if(sendMsg < 0) {
    std::cerr << "Error occured";
  } else {
    std::cout << message;
  }

  // extract URL path
  std::string client_message(1024, '\0');

  ssize_t accept = recv(client, (void *)&client_message[0], client_message.max_size(), 0);

  if(accept < 0) {
    std::cout << "Client message is short";
    close(client);
    close(server_fd);
    return 1;
  }

  std::string response = client_message.starts_with("GET / HTTP/1.1\r\n") ? "HTTP/1.1 200 OK\r\n\r\n" : "HTTP/1.1 404 NOT FOUND\r\n\r\n";

  close(server_fd);
  return 0;
}
