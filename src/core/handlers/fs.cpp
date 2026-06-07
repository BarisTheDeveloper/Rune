#include "core/handlers/fs.hpp"
#include "ipc/server.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace rune::handlers {

static std::string s_appDir;

// Resolve path relative to app directory
// Block traversal above appDir for security
static std::string resolvePath(const std::string& rawPath) {
  // If absolute, resolve relative to appDir
  if (rawPath.empty()) return s_appDir;

  fs::path resolved;
  if (rawPath[0] == '/') {
    resolved = fs::path(rawPath);
  } else if (rawPath[0] == '~') {
    // Expand ~ to HOME
    const char* home = getenv("HOME");
    if (home) {
      std::string expanded = std::string(home) + rawPath.substr(1);
      resolved = fs::path(expanded);
    } else {
      resolved = fs::path(s_appDir) / rawPath;
    }
  } else {
    resolved = fs::path(s_appDir) / rawPath;
  }

  // Canonicalize (resolves .. and symlinks)
  std::error_code ec;
  fs::path canonical = fs::weakly_canonical(resolved, ec);
  if (ec) return resolved.string(); // fallback

  return canonical.string();
}

void registerFsHandlers(rune::ipc::Server& server, const std::string& appDir) {
  s_appDir = appDir;

  // fs.readDir
  server.on("fs.readDir", [](const std::string&, const std::string& params) -> std::string {
    try {
      json p = json::parse(params);
      std::string dirPath = resolvePath(p.value("path", "."));
      json result = json::array();

      for (const auto& entry : fs::directory_iterator(dirPath)) {
        json item;
        item["name"] = entry.path().filename().string();
        item["isDir"] = entry.is_directory();
        item["isFile"] = entry.is_regular_file();
        item["isSymlink"] = entry.is_symlink();
        result.push_back(item);
      }

      return result.dump();
    } catch (const std::exception& e) {
      json err;
      err["error"] = std::string("fs.readDir failed: ") + e.what();
      return err.dump();
    }
  });

  // fs.readFile
  server.on("fs.readFile", [](const std::string&, const std::string& params) -> std::string {
    try {
      json p = json::parse(params);
      std::string filePath = resolvePath(p.value("path", ""));
      std::ifstream file(filePath);
      if (!file.is_open()) {
        json err;
        err["error"] = "Cannot open file: " + filePath;
        return err.dump();
      }

      std::stringstream buffer;
      buffer << file.rdbuf();
      return json(buffer.str()).dump(); // JSON string
    } catch (const std::exception& e) {
      json err;
      err["error"] = std::string("fs.readFile failed: ") + e.what();
      return err.dump();
    }
  });

  // fs.writeFile
  server.on("fs.writeFile", [](const std::string&, const std::string& params) -> std::string {
    try {
      json p = json::parse(params);
      std::string filePath = resolvePath(p.value("path", ""));
      std::string data = p.value("data", "");

      std::ofstream file(filePath);
      if (!file.is_open()) {
        json err;
        err["error"] = "Cannot write file: " + filePath;
        return err.dump();
      }

      file << data;
      file.close();

      json result;
      result["ok"] = true;
      return result.dump();
    } catch (const std::exception& e) {
      json err;
      err["error"] = std::string("fs.writeFile failed: ") + e.what();
      return err.dump();
    }
  });

  // fs.exists
  server.on("fs.exists", [](const std::string&, const std::string& params) -> std::string {
    try {
      json p = json::parse(params);
      std::string filePath = resolvePath(p.value("path", ""));
      return fs::exists(filePath) ? "true" : "false";
    } catch (...) {
      return "false";
    }
  });

  // fs.stat
  server.on("fs.stat", [](const std::string&, const std::string& params) -> std::string {
    try {
      json p = json::parse(params);
      std::string filePath = resolvePath(p.value("path", ""));
      std::error_code ec;
      auto status = fs::status(filePath, ec);

      if (ec) {
        json err;
        err["error"] = "Cannot stat: " + filePath;
        return err.dump();
      }

      json result;
      result["exists"] = fs::exists(status);
      result["isDir"] = fs::is_directory(status);
      result["isFile"] = fs::is_regular_file(status);
      result["isSymlink"] = fs::is_symlink(status);
      if (fs::is_regular_file(status)) {
        result["size"] = fs::file_size(filePath, ec);
      }
      return result.dump();
    } catch (const std::exception& e) {
      json err;
      err["error"] = std::string("fs.stat failed: ") + e.what();
      return err.dump();
    }
  });

  std::cout << "[RUNE] FS handlers registered (appDir: " << appDir << ")" << std::endl;
}

} // namespace rune::handlers
