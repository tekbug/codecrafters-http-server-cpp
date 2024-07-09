#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
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

  // Extract URL path, Parse the URL, Respond with RequestBody, and Respond with 404 for error pages

  // setting buffer
  char buffer[1024];

  // reading client request, and setting the byte to the buffer and also setting the length of the buffer to 1024
  int client_rd = read(client_fd, buffer, 1024);

  // empty buffer init
  buffer[client_rd] = '\0';

  std::string path;

  std::string request(buffer);

  ssize_t req_method = request.find(' ');

  /*
   * So what we basically expect is to get "RequestMethod path/{string} Status" for eg: "GET / HTTP/1.1 200 OK"
   * in order to be able to find this, we gotta parse the request and find the gaps and stuff
   */
  if(req_method != std::string::npos) {
    auto start_ptr = req_method + 1;
    auto end_ptr = request.find(' ', start_ptr);

    if(end_ptr != std::string::npos) {
      path = request.substr(start_ptr, end_ptr - start_ptr);
    }
  }

  // byte that is going to be sent to the server
  ssize_t bsend;

  if(path == "/") {
    std::string response = "HTTP/1.1 200 OK\r\n\r\n";
    bsend = send(client_fd, response.c_str(), response.size(), 0);
  } else if (path.find("/echo/") == 0) {
    std::string req_path = path.substr(6);
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(req_path.size()) + "\r\n\r\n" + req_path;
    bsend = send(client_fd, response.c_str(), response.size(), 0);
  } else {
    std::string response = "HTTP/1.1 404 Not Found\r\n\r\n";
    bsend = send(client_fd, response.c_str(), response.size(), 0);
  }

  if(bsend < 0) {
    std::cerr << "Error occurred sending request to the server";
    close(client_fd);
    close(server_fd);
  }

  close(client_fd);
  close(server_fd);
  return 0;
}
