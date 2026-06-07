#include "core/handlers/process.hpp"
#include "ipc/server.hpp"
#include <nlohmann/json.hpp>
#include <cstdio>
#include <array>
#include <memory>
#include <iostream>
#include <sys/wait.h>

using json = nlohmann::json;

namespace rune::handlers {

static std::string execCommand(const std::string& cmd, int timeoutSec = 10) {
  std::array<char, 128> buffer;
  std::string result;

  // Use popen to execute and capture stdout
  std::unique_ptr<FILE, decltype(&pclose)> pipe(
    popen(cmd.c_str(), "r"), pclose
  );

  if (!pipe) {
    json err;
    err["error"] = "Failed to execute command";
    return err.dump();
  }

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  int exitCode = pclose(pipe.release());
  // WEXITSTATUS extracts actual exit code from wait status
  int actualExit = WEXITSTATUS(exitCode);

  json out;
  out["stdout"] = result;
  out["stderr"] = "";   // popen only captures stdout
  out["exitCode"] = actualExit;
  out["ok"] = (actualExit == 0);

  return out.dump();
}

void registerProcessHandlers(rune::ipc::Server& server) {
  server.on("process.exec", [](const std::string&, const std::string& params) -> std::string {
    try {
      json p = json::parse(params);
      std::string cmd = p.value("cmd", "");
      int timeout = p.value("timeout", 10);

      if (cmd.empty()) {
        json err;
        err["error"] = "No command provided";
        return err.dump();
      }

      return execCommand(cmd, timeout);
    } catch (const std::exception& e) {
      json err;
      err["error"] = std::string("process.exec failed: ") + e.what();
      return err.dump();
    }
  });

  std::cout << "[RUNE] Process handlers registered" << std::endl;
}

} // namespace rune::handlers
