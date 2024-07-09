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
#include <thread>
#include <vector>
#include <fstream>

/*
 * Most of the structured comments you read on this is made by the bots. Will update after finishing up the entire thing.
 * Feel free to use this codebase as however you see fit. Just for learning, and shortcutting a lot of things.
 * By tekbug
 */



void concurrent_users(int client, const std::string &dir) {
  char buffer[4096];
  int client_rd = read(client, buffer, 4096);
  buffer[client_rd] = '\0';

  std::string path;
  std::string request(buffer);


  ssize_t req_method = request.find(' ');
  std::string method;
  if(req_method !=std::string::npos) {
    method = request.substr(0, req_method);
  }

  if(req_method != std::string::npos) {
    ssize_t start_pos = req_method + 1;
    ssize_t end_pos = request.find(' ', start_pos);

    if(end_pos != std::string::npos) {
      path = request.substr(start_pos, end_pos - start_pos);
    }
  }

  std::string body;
  ssize_t body_start = request.find("\r\n\r\n");
  if (body_start != std::string::npos) {
    body = request.substr(body_start + 4);
  }

  ssize_t bsend;
  ssize_t agent = request.find("User-Agent:");

  if(path == "/") {
    std::string response = "HTTP/1.1 200 OK\r\n\r\n";
    bsend = send(client, response.c_str(), response.size(), 0);
  }

  else if (path.find("/echo/") == 0) {
    std::string req_path = path.substr(6);
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(req_path.size()) + "\r\n\r\n" + req_path;
    bsend = send(client, response.c_str(), response.size(), 0);
  }

  else if (path.find("/user-agent") == 0) {
    std::string agent_str;
    if(agent != std::string::npos) {
      ssize_t end = request.find("\r\n", agent);
      if(end != std::string::npos) {
        agent_str = request.substr(agent + 12, end - (agent + 12));
      }
    }
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(agent_str.size()) + "\r\n\r\n" + agent_str;
    std::cout << response;
    bsend = send(client, response.c_str(), response.size(), 0);
  }

  else if (method == "POST" && path.find("/files") == 0) {
    std::string req_path = dir + path.substr(7);
    std::ofstream wStream(req_path);
    if (wStream.good()) {
      wStream << body;
      wStream.close();
      std::string response = "HTTP/1.1 201 Created\r\n\r\n";
      bsend = send(client, response.c_str(), response.size(), 0);
    }
  }

  else if(path.find("/files/") == 0) {
    std::string req_path = dir + path.substr(6);
    std::ifstream rStream(req_path);
    if(rStream.good()) {
      std::stringstream ifs_file_content;
      ifs_file_content << rStream.rdbuf();
      std::stringstream respond("");
      std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: " + std::to_string(ifs_file_content.str().length()) + "\r\n\r\n" + ifs_file_content.str() + "\r\n";
      send(client, response.c_str(), response.size(), 0);
    } else {
      std::string response="HTTP/1.1 404 Not Found\r\n\r\n";
      send(client, response.c_str(), response.size(), 0);
    }
  }

  else {
    std::string response = "HTTP/1.1 404 Not Found\r\n\r\n";
    bsend = send(client, response.c_str(), response.size(), 0);
  }

  if(bsend < 0) {
    std::cerr << "Error occurred sending request to the server";
    close(client);
  }

  close(client);
}

int main(int argc, char **argv) {

  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  /*
   * the dir is used to bind the flag to the request
   */

  std::string dir;

  if(argc == 3 && strcmp(argv[1], "--directory") == 0) {
    dir = argv[2];
  }

  /**
   * file descriptor -> fd (explanation from our lords and saviours - codellama-7b, and GPT-4o)
   *
   * In computing, particularly in the context of developing an HTTP server, "fd" stands for "file descriptor."
   * A file descriptor is an integer that uniquely identifies an open file in the operating system.
   * It is a low-level construct used for accessing files, sockets, pipes, and other input/output resources.
   *
   * So the below function is initializing a socket in the server and assigning a unique int to it.
   *
   * Explaining the first part of the function and what they does, as far as I understood.
   *
   * 1. The setsockopt function is used to set the SO_REUSEADDR option on the socket.
   *    This allows the socket to bind to an address that is already in use, which is useful for
   *    quickly restarting the server without waiting for the operating system to release the port.
   *
   * 2. A sockaddr_in structure is defined and configured to specify the server's address and port.
   *    - sin_family is set to AF_INET, indicating the use of IPv4.
   *    - sin_addr.s_addr is set to INADDR_ANY, allowing the server to accept connections on any available interface.
   *    - sin_port is set to port 4221, converted to network byte order using the htons function.
   *
   * 3. The bind function associates the socket with the specified address and port. If binding fails,
   *    an error message is printed, the socket is closed, and the program exits with a return value of 1.
   *
   * 4. The listen function marks the socket as a passive socket that will accept incoming connection requests.
   *    The connection_backlog variable specifies the maximum number of pending connections that can be queued.
   *    If listen fails, an error message is printed, the socket is closed, and the program exits with a return value of 1.
   *
   * 5. A sockaddr_in structure and an integer variable are prepared to hold the address information
   *    and length of a client connection. These will be used when accepting connections.
   *
   *
   * "The htons() function converts the unsigned short integer hostshort from host byte order to network byte order. "
   * "The htonl() function converts the unsigned integer hostlong from host byte order to network byte order. "
   * 		- https://linux.die.net/man/3/htons
   *
   * struct sockaddr_in is a struct from in.h that has the following data:
   * 			sin_port -> Port number
   * 			sin_addr -> Internet address
   * 			sin_zero -> Padding to make the structure the same size as sockaddr
   */
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

  /**
   * According to IBM:
   *
   * 		"The bind() function binds a unique local name to the socket with descriptor socket.
   * 		After calling socket(), a descriptor does not have a name associated with it.
   * 		However, it does belong to a particular address family as specified when socket()
   * 		is called. The exact format of a name depends on the address family."
   *
   * 	- https://www.ibm.com/docs/en/zos/2.3.0?topic=functions-bind-bind-name-socket
   *
   * In other words: when you first create a socket using `socket()` function, the socket is
   * not yet associated with a specific address or port.
   *
   * Params:
   * -> server_fd is the socket identifier that we created before.
   * -> serv_addr is the struct that we specified. serv_addr is casted from
   * sockaddr_in to sockaddr since sockaddr is the generic struct for socket addresses.
   * -> The size of the socket address structure.
   *
   * The function should return 0 if the binding was successful
   */

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

  //TODO: Send a 200 OK, Extract URL path, Parse the URL, Respond with RequestBody, Respond with 404 for invalid URLs, Read files

  /*
     * This code handles reading and parsing the HTTP request received from the client.
     * It extracts the URL path from the request and prepares to respond accordingly.
     *
     * 1. char buffer[1024]: Creates a buffer to store the incoming HTTP request data (up to 1024 bytes).
     *
     * 2. int client_rd = read(client_fd, buffer, 1024): Reads data from the client socket (`client_fd`)
     *    into the `buffer`, up to 1024 bytes. `client_rd` stores the number of bytes read.
     *
     * 3. buffer[client_rd] = '\0': Null-terminates the buffer to ensure the string is properly terminated
     *    for string operations.
     *
     * 4. std::string path: Declares a string to store the extracted URL path from the HTTP request.
     *
     * 5. std::string request(buffer): Constructs a string `request` from the buffer data, representing
     *    the entire HTTP request.
     *
     * 6. ssize_t req_method = request.find(' '): Finds the position of the first space character (' ')
     *    in the `request` string, which marks the end of the HTTP method (e.g., GET, POST).
     *
     * 7. if (req_method != std::string::npos): Checks if a space character was found in `request`.
     *
     *    - auto start_ptr = req_method + 1;: Sets `start_ptr` to the position right after the HTTP method.
     *
     *    - auto end_ptr = request.find(' ', start_ptr);: Finds the next space character after `start_ptr`,
     *      which marks the end of the URL path and potentially the beginning of the HTTP version or other data.
     *
     *    - if (end_ptr != std::string::npos): Checks if a second space character was found in `request`.
     *
     *      - path = request.substr(start_ptr, end_ptr - start_ptr);: Extracts the substring from `request`
     *        starting at `start_ptr` and extending to `end_ptr - start_ptr`, which represents the URL path.
 */

  std::vector<std::thread> threads;

  while(true) {
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);

    if(client_fd < 0) {
      std::cerr << "error occurred in client connection";
      close(client_fd);
      close(server_fd);
      return 1;
    }
    threads.emplace_back([client_fd, dir]() {
      concurrent_users(client_fd, dir);
    });
  }
}
