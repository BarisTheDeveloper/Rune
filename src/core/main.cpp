#include <iostream>
#include <csignal>
#include <gtk/gtk.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <unistd.h>
#include "ipc/server.hpp"
#include "ui/window.hpp"
#include "core/handlers/fs.hpp"
#include "core/handlers/process.hpp"

using json = nlohmann::json;

static rune::ipc::Server* g_ipc = nullptr;

// GTK idle callback — runs IPC tick from GTK main loop
static gboolean ipc_tick_callback(gpointer) {
  if (g_ipc) {
    g_ipc->tick();
  }
  return G_SOURCE_CONTINUE; // keep calling
}

int main(int argc, char* argv[]) {
  std::cout << "[RUNE] Core starting..." << std::endl;

  // Init GTK
  gtk_init(&argc, &argv);

  // Create window
  rune::ui::Window window;
  if (!window.create(900, 650, "Rune")) {
    std::cerr << "[RUNE] Failed to create window" << std::endl;
    return 1;
  }

  // Start IPC server
  rune::ipc::Server ipc;
  g_ipc = &ipc;
  std::string socketPath = "/tmp/rune-" + std::to_string(getpid()) + ".sock";

  if (!ipc.start(socketPath)) {
    return 1;
  }

  // Determine app directory (where the app's project files are)
  std::string appDir = std::filesystem::current_path().string();
  // If launched by CLI with --app-dir, use that
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--app-dir" && i + 1 < argc) {
      appDir = argv[++i];
      break;
    }
  }

  // Register handlers
  rune::handlers::registerFsHandlers(ipc, appDir);
  rune::handlers::registerProcessHandlers(ipc);
  ipc.on("system.version", [](const std::string&, const std::string&) -> std::string {
    return "\"0.1.0\"";
  });

  ipc.on("system.platform", [](const std::string&, const std::string&) -> std::string {
    return "\"linux\"";
  });

  ipc.on("system.arch", [](const std::string&, const std::string&) -> std::string {
    return "\"x86_64\"";
  });

  ipc.on("ping", [](const std::string&, const std::string&) -> std::string {
    return "\"pong\"";
  });

  // Wire JS bridge → IPC
  window.onBridgeMessage([&ipc](const std::string& jsMessage) -> std::string {
    try {
      auto msg = json::parse(jsMessage);
      std::string method = msg.value("method", "");
      std::string params = msg.contains("params") ? msg["params"].dump() : "{}";
      int id = msg.value("id", -1);

      std::string result;
      if (method == "system.version") result = "\"0.1.0\"";
      else if (method == "system.platform") result = "\"linux\"";
      else if (method == "system.arch") result = "\"x86_64\"";
      else if (method == "ping") result = "\"pong\"";
      else result = R"({"error":"Unknown method: )" + method + R"("})";

      // Format as JS callback
      json resp;
      resp["id"] = id;
      try {
        resp["result"] = json::parse(result);
      } catch (...) {
        resp["result"] = result;
      }

      return "rune._resolve(" + std::to_string(id) + ", null, " + resp["result"].dump() + ")";
    } catch (...) {
      return "";
    }
  });

  // Load initial HTML
  const char* html = R"HTML(<!DOCTYPE html>
<html><head><meta charset="utf-8"><title>Rune</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body { font-family: system-ui, sans-serif; background: #0d0d0d; color: #e0e0e0;
         display: flex; flex-direction: column; align-items: center;
         justify-content: center; height: 100vh; }
  h1 { font-size: 2.5em; color: #8b3ae0; margin-bottom: 8px; }
  p { font-size: 1em; color: #888; margin-bottom: 24px; }
  button { background: #8b3ae0; color: #fff; border: none; padding: 10px 24px;
           font-size: 1em; border-radius: 6px; cursor: pointer; margin: 4px; }
  button:hover { background: #a050f0; }
  pre { margin-top: 16px; padding: 16px; background: #1a1a1a; border-radius: 8px;
        font-size: 0.85em; max-width: 600px; overflow-x: auto; }
</style></head><body>
<h1>Rune</h1>
<p>Build native. Stay fast.</p>
<button onclick="testPing()">Test Ping</button>
<button onclick="testVersion()">Test Version</button>
<button onclick="testPlatform()">Test Platform</button>
<pre id="output">Ready.</pre>
<script>
async function testPing() {
  document.getElementById('output').textContent = 'ping...';
  const r = await window.rune.call('ping');
  document.getElementById('output').textContent = JSON.stringify(r, null, 2);
}
async function testVersion() {
  const r = await window.rune.call('system.version');
  document.getElementById('output').textContent = 'Rune v' + r;
}
async function testPlatform() {
  const r = await window.rune.call('system.platform');
  document.getElementById('output').textContent = 'Platform: ' + r;
}
</script>
</body></html>)HTML";

  window.loadHtml(html);

  // Schedule IPC ticks on GTK idle
  g_idle_add(ipc_tick_callback, nullptr);

  std::cout << "[RUNE] Ready. Socket: " << socketPath << std::endl;

  // Run GTK main loop (blocks until window closes)
  window.run();

  std::cout << "[RUNE] Shutting down." << std::endl;
  return 0;
}
