#ifndef RUNE_IPC_SERVER_HPP
#define RUNE_IPC_SERVER_HPP

#include <string>
#include <functional>
#include <vector>

namespace rune::ipc {

struct Client {
  int fd;
  std::string buffer;  // partial reads accumulated here
};

using MessageHandler = std::function<std::string(const std::string& method, const std::string& params)>;

class Server {
public:
  Server();
  ~Server();

  // Start listening on a Unix Domain Socket
  bool start(const std::string& socketPath);

  // Process one event loop tick (accept + read + dispatch + respond)
  // Returns false on fatal error
  bool tick();

  // Register handler for an IPC method
  void on(const std::string& method, MessageHandler handler);

  // Number of connected clients
  int clientCount() const { return static_cast<int>(m_clients.size()); }

  const std::string& socketPath() const { return m_socketPath; }

private:
  int m_serverFd;
  std::string m_socketPath;
  std::vector<Client> m_clients;

  void acceptClient();
  void processClient(Client& client);
  std::string dispatch(const std::string& method, const std::string& params);
  void sendResponse(int fd, const std::string& json);
  void removeClient(int fd);
};

} // namespace rune::ipc

#endif
