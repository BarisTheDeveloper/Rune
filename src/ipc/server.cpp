#include "ipc/server.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace rune::ipc {

Server::Server() : m_serverFd(-1) {}

Server::~Server() {
  if (m_serverFd >= 0) {
    close(m_serverFd);
    unlink(m_socketPath.c_str());
  }
}

bool Server::start(const std::string& socketPath) {
  m_socketPath = socketPath;

  // Remove stale socket file
  unlink(socketPath.c_str());

  m_serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (m_serverFd < 0) {
    std::cerr << "[RUNE] Failed to create socket" << std::endl;
    return false;
  }

  struct sockaddr_un addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

  if (bind(m_serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    std::cerr << "[RUNE] Failed to bind socket: " << socketPath << std::endl;
    close(m_serverFd);
    return false;
  }

  if (listen(m_serverFd, 5) < 0) {
    std::cerr << "[RUNE] Failed to listen" << std::endl;
    close(m_serverFd);
    return false;
  }

  std::cout << "[RUNE] IPC listening on " << socketPath << std::endl;
  return true;
}

void Server::acceptClient() {
  int clientFd = accept(m_serverFd, nullptr, nullptr);
  if (clientFd < 0) return;

  // Set non-blocking
  int flags = fcntl(clientFd, F_GETFL, 0);
  fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

  Client client;
  client.fd = clientFd;
  m_clients.push_back(client);

  std::cout << "[RUNE] Client connected (fd=" << clientFd << ")" << std::endl;
}

void Server::processClient(Client& client) {
  char buf[8192];
  ssize_t n = read(client.fd, buf, sizeof(buf) - 1);

  if (n <= 0) {
    // Client disconnected
    removeClient(client.fd);
    return;
  }

  buf[n] = '\0';
  client.buffer += buf;

  // Process complete JSON lines (newline-delimited)
  size_t pos;
  while ((pos = client.buffer.find('\n')) != std::string::npos) {
    std::string line = client.buffer.substr(0, pos);
    client.buffer.erase(0, pos + 1);

    if (line.empty()) continue;

    try {
      json msg = json::parse(line);

      std::string method = msg.value("method", "");
      std::string params = msg.contains("params") ? msg["params"].dump() : "{}";
      int id = msg.value("id", -1);

      std::string result = dispatch(method, params);

      // Build response
      json response;
      response["id"] = id;
      try {
        response["result"] = json::parse(result);
      } catch (...) {
        response["result"] = result;
      }

      sendResponse(client.fd, response.dump());
    } catch (const json::parse_error& e) {
      json error;
      error["id"] = -1;
      error["error"] = std::string("Parse error: ") + e.what();
      sendResponse(client.fd, error.dump());
    }
  }
}

void Server::sendResponse(int fd, const std::string& json) {
  std::string msg = json + "\n";
  write(fd, msg.c_str(), msg.size());
}

void Server::removeClient(int fd) {
  std::cout << "[RUNE] Client disconnected (fd=" << fd << ")" << std::endl;
  close(fd);
  m_clients.erase(
    std::remove_if(m_clients.begin(), m_clients.end(),
      [fd](const Client& c) { return c.fd == fd; }),
    m_clients.end()
  );
}

bool Server::tick() {
  if (m_serverFd < 0) return false;

  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(m_serverFd, &readfds);
  int maxFd = m_serverFd;

  for (const auto& client : m_clients) {
    FD_SET(client.fd, &readfds);
    if (client.fd > maxFd) maxFd = client.fd;
  }

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000; // 100ms timeout

  int ready = select(maxFd + 1, &readfds, nullptr, nullptr, &tv);
  if (ready < 0) {
    std::cerr << "[RUNE] select() error" << std::endl;
    return false;
  }

  if (FD_ISSET(m_serverFd, &readfds)) {
    acceptClient();
  }

  // Process each client (copy fd list because processClient can modify m_clients)
  for (size_t i = 0; i < m_clients.size(); ) {
    int fd = m_clients[i].fd;
    if (FD_ISSET(fd, &readfds)) {
      processClient(m_clients[i]);
      // If client was removed, don't increment i (vector shifted)
      if (i < m_clients.size() && m_clients[i].fd == fd) {
        ++i;
      }
    } else {
      ++i;
    }
  }

  return true;
}

// --- Dispatcher (inline for now, moves to dispatcher.cpp later) ---

static std::unordered_map<std::string, MessageHandler> g_handlers;

void Server::on(const std::string& method, MessageHandler handler) {
  g_handlers[method] = handler;
}

std::string Server::dispatch(const std::string& method, const std::string& params) {
  auto it = g_handlers.find(method);
  if (it != g_handlers.end()) {
    return it->second(method, params);
  }

  json error;
  error["error"] = "Unknown method: " + method;
  return error.dump();
}

} // namespace rune::ipc
