# Rune — Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** Build a Linux-first desktop application runtime combining TypeScript frontend with a C++ native core, using Unix Domain Sockets + JSON for IPC.

**Architecture:** Three-layer stack — C++ runtime core (window management, IPC server, system APIs) → IPC bridge (Unix Domain Socket + JSON-RPC-like protocol) → TypeScript/Node.js CLI + frontend API.

**Tech Stack:** C++17 (CMake), TypeScript (Node.js + Commander.js), WebKitGTK (window rendering), Unix Domain Sockets (IPC), JSON (nlohmann/json)

---

## Phase 0 — Project Scaffolding

### Task 0.1: Set up CMake build system for C++ core
**Objective:** Replace empty Makefile with CMake, split rune.cpp into proper src/ structure
**Files:**
- Modify: `Makefile` → replace with `CMakeLists.txt`
- Delete: `rune.cpp` (moves into src/core/)
- Create: `src/core/main.cpp`
- Create: `src/core/CMakeLists.txt`

**Step 1: Root CMakeLists.txt**
```cmake
cmake_minimum_required(VERSION 3.20)
project(rune-core VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

# Dependencies
pkg_check_modules(WEBKIT2 REQUIRED webkit2gtk-4.1)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(JSON IMPORTED_TARGET nlohmann_json)

add_subdirectory(src)
```

**Step 2: src/CMakeLists.txt**
```cmake
add_executable(rune
  core/main.cpp
)

target_include_directories(rune PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${WEBKIT2_INCLUDE_DIRS}
  ${GTK3_INCLUDE_DIRS}
)

target_link_libraries(rune PRIVATE
  ${WEBKIT2_LIBRARIES}
  ${GTK3_LIBRARIES}
  pthread
)

target_compile_options(rune PRIVATE -Wall -Wextra -O2)
```

**Step 3: Update .gitignore**
```
build/
node_modules/
dist/
*.o
.DS_Store
```

**Verification:** `mkdir -p build && cd build && cmake .. && make -j$(nproc)`

---

### Task 0.2: Set up TypeScript CLI project
**Objective:** Create the `rune` CLI package with Commander.js
**Files:**
- Create: `cli/package.json`
- Create: `cli/tsconfig.json`
- Create: `cli/src/index.ts`
- Create: `cli/src/commands/init.ts`
- Create: `cli/src/commands/dev.ts`
- Create: `cli/src/commands/build.ts`
- Create: `cli/src/commands/run.ts`
- Create: `cli/src/commands/doctor.ts`
- Create: `cli/src/commands/add.ts`
- Create: `cli/version.rune`

**cli/package.json:**
```json
{
  "name": "rune-cli",
  "version": "0.1.0",
  "type": "module",
  "main": "./dist/index.js",
  "bin": { "rune": "./dist/index.js" },
  "files": ["dist"],
  "scripts": {
    "build": "tsc",
    "dev": "tsx src/index.ts"
  },
  "dependencies": {
    "commander": "^13.0.0",
    "chalk": "^5.4.0"
  },
  "devDependencies": {
    "@types/node": "^22.0.0",
    "typescript": "^5.7.0",
    "tsx": "^4.19.0"
  }
}
```

**cli/tsconfig.json:**
```json
{
  "compilerOptions": {
    "target": "ES2022",
    "module": "Node16",
    "moduleResolution": "Node16",
    "outDir": "./dist",
    "rootDir": "./src",
    "strict": true,
    "noUncheckedIndexedAccess": true,
    "verbatimModuleSyntax": true,
    "resolveJsonModule": true,
    "esModuleInterop": true,
    "skipLibCheck": true
  },
  "include": ["src/**/*.ts"]
}
```

**cli/src/index.ts:**
```typescript
#!/usr/bin/env node
import { Command } from "commander";
import { runInit } from "./commands/init.js";
import { runDev } from "./commands/dev.js";
import { runBuild } from "./commands/build.js";
import { runRun } from "./commands/run.js";
import { runDoctor } from "./commands/doctor.js";
import { runAdd } from "./commands/add.js";

const program = new Command();

program
  .name("rune")
  .description("Rune — Build native. Stay fast.")
  .version("0.1.0");

program
  .command("init [name]")
  .description("Initialize a new Rune project")
  .action(async (name?: string) => {
    await runInit(name);
  });

program
  .command("dev")
  .description("Start development server")
  .action(async () => {
    await runDev();
  });

program
  .command("build")
  .description("Build application for distribution")
  .action(async () => {
    await runBuild();
  });

program
  .command("run")
  .description("Run the built application")
  .action(async () => {
    await runRun();
  });

program
  .command("doctor")
  .description("Check environment for required dependencies")
  .action(async () => {
    await runDoctor();
  });

program
  .command("add <plugin>")
  .description("Add a Rune plugin")
  .action(async (plugin: string) => {
    await runAdd(plugin);
  });

program.parse();
```

**Verification:** `cd cli && npm install && npm run build && node dist/index.js --help`

---

### Task 0.3: Create project structure conventions (init template)
**Objective:** `rune init` generates a standard project layout
**Files:**
- Create: `cli/src/templates/project/package.json.tmpl`
- Create: `cli/src/templates/project/tsconfig.json.tmpl`
- Create: `cli/src/templates/project/index.html.tmpl`
- Create: `cli/src/templates/project/src/main.ts.tmpl`
- Create: `cli/src/templates/project/rune.config.json.tmpl`

**Template structure:**
```
my-rune-app/
├── src/
│   ├── main.ts          # Entry point
│   ├── app.ts           # App component
│   └── style.css
├── public/
│   └── index.html
├── rune.config.json     # Project config
├── package.json
└── tsconfig.json
```

**Verification:** `cd /tmp && rune init test-app && ls test-app/`

---

## Phase 1 — C++ IPC Core

### Task 1.1: Unix Domain Socket server
**Objective:** Create IPC server that listens on a UDS, accepts JSON-RPC-like requests
**Files:**
- Create: `src/ipc/server.hpp`
- Create: `src/ipc/server.cpp`

**Design:**
- Socket path: `/tmp/rune-<pid>.sock`
- Protocol: JSON lines, one request/response per line
- Request: `{"id": 1, "method": "system.version", "params": {}}`
- Response: `{"id": 1, "result": "0.1.0"}` or `{"id": 1, "error": "..."}`

**Verification:** Manual test — start server, connect with `nc -U /tmp/rune-*.sock`, send valid/invalid JSON

---

### Task 1.2: Message dispatcher + handler registry
**Objective:** Route incoming IPC messages to registered handlers
**Files:**
- Create: `src/ipc/dispatcher.hpp`
- Create: `src/ipc/dispatcher.cpp`
- Create: `src/core/handlers/system.cpp`

**Design:**
- `Dispatcher` holds `map<string, HandlerFn>` — method name → handler
- Handlers are typed: `json handler(const json& params)`
- Error handling: unknown method → error response, invalid params → error response

**Verification:** Register `system.version` handler, send IPC request, verify response

---

### Task 1.3: Non-blocking IPC + event loop
**Objective:** `select()` / `epoll` based event loop that handles multiple simultaneous connections
**Files:**
- Create: `src/ipc/loop.hpp`
- Create: `src/ipc/loop.cpp`

**Design:**
- Single-threaded epoll loop
- Accept new connections, buffer reads until complete JSON line
- One connection = one webview (for later)
- Clean up disconnected clients

**Verification:** Connect 3 clients simultaneously, send concurrent requests, verify all get responses

---

## Phase 2 — Window System

### Task 2.1: WebKitGTK window creation
**Objective:** Create a native GTK window with an embedded WebKit webview
**Files:**
- Create: `src/ui/window.hpp`
- Create: `src/ui/window.cpp`

**Design:**
- Creates GTK window with title, default 800x600 size
- Embeds WebKitGTK WebView
- Loads `index.html` from the app's project directory
- Window delegates resize/close events to IPC

**Verification:** Run binary, window appears showing 'Hello from Rune!' HTML

---

### Task 2.2: JavaScript bridge (WebKit → IPC)
**Objective:** Let frontend JS call native APIs via `window.rune.call(method, params)`
**Files:**
- Modify: `src/ui/window.cpp` — register WebKit script message handler
- Create: `cli/src/templates/project/public/rune-bridge.js`

**Design:**
- WebKit `window.webkit.messageHandlers.rune.postMessage(JSON)` sends to C++
- C++ side: `webkit_user_content_manager_register_script_message_handler()`
- C++ forwards message to IPC handler, returns result to JS callback
- Frontend API: `const result = await window.rune.call("system.version")`

**rune-bridge.js (injected into every page):**
```javascript
window.rune = {
  _id: 0,
  _pending: {},
  call(method, params = {}) {
    return new Promise((resolve, reject) => {
      const id = ++this._id;
      this._pending[id] = { resolve, reject };
      window.webkit.messageHandlers.rune.postMessage(
        JSON.stringify({ id, method, params })
      );
    });
  },
  _resolve(id, error, result) {
    const p = this._pending[id];
    if (!p) return;
    delete this._pending[id];
    if (error) p.reject(new Error(error));
    else p.resolve(result);
  }
};
```

**Verification:** Load HTML page, `await window.rune.call("system.version")` in DevTools returns version

---

### Task 2.3: Run loop integration (GTK + epoll in same thread)
**Objective:** Integrate GTK main loop with IPC epoll — both must coexist
**Files:**
- Modify: `src/core/main.cpp`
- Modify: `src/ipc/loop.cpp`

**Design:**
- GTK runs on `gtk_main()` (blocking)
- Register epoll fd with `g_idle_add()` or use GTK's `g_io_add_watch()` for socket
- OR: Run GTK in separate thread, IPC in main thread
- Prefer: single-thread with `g_io_add_watch()` for cleanest integration

**Verification:** Window stays responsive while IPC requests are handled

---

## Phase 3 — System APIs

### Task 3.1: System info handlers
**Objective:** Implement `system.version`, `system.platform`, `system.arch` handlers
**Files:**
- Create: `src/core/handlers/system.cpp` (if not created in 1.2)
- Create: `src/core/handlers/system.hpp`

**Methods:**
- `system.version` → returns C++ project version
- `system.platform` → returns "linux"
- `system.arch` → returns `uname -m`
- `system.homeDir` → returns `$HOME`

**Verification:** Call each from JS bridge, verify responses

---

### Task 3.2: File system handlers
**Objective:** Implement `fs.readDir`, `fs.readFile`, `fs.writeFile`, `fs.exists`, `fs.stat`
**Files:**
- Create: `src/core/handlers/fs.cpp`
- Create: `src/core/handlers/fs.hpp`

**Security:** Paths resolved relative to app root, no `..` traversal beyond project dir
**Verification:** Read/write/list files from JS, verify no traversal

---

### Task 3.3: Process execution handler
**Objective:** Implement `process.exec` — run shell commands, capture stdout/stderr, return code
**Files:**
- Modify: `src/core/handlers/fs.cpp` → add process handler or new file
- Create: `src/core/handlers/process.cpp`

**Design:**
- `fork()` + `exec()` with `popen()` or manual pipe dup
- Return `{stdout, stderr, exitCode}`
- Timeout support (SIGALRM or `waitpid` with WNOHANG + select)
- Whitelist/confirmation for dangerous commands? (Phase 3 decision, skip for now)

**Verification:** `rune.process.exec("ls -la")` from JS, verify output

---

## Phase 4 — TypeScript API Package

### Task 4.1: @rune/api npm package
**Objective:** Create the TypeScript API package that wraps the bridge
**Files:**
- Create: `packages/api/package.json`
- Create: `packages/api/tsconfig.json`
- Create: `packages/api/src/system.ts`
- Create: `packages/api/src/fs.ts`
- Create: `packages/api/src/process.ts`
- Create: `packages/api/src/index.ts`

**API design:**
```typescript
// packages/api/src/system.ts
export const system = {
  version: () => runeCall<string>("system.version"),
  platform: () => runeCall<string>("system.platform"),
  arch: () => runeCall<string>("system.arch"),
  homeDir: () => runeCall<string>("system.homeDir"),
};

// packages/api/src/fs.ts
export const fs = {
  readDir: (path: string) => runeCall<DirEntry[]>("fs.readDir", { path }),
  readFile: (path: string) => runeCall<string>("fs.readFile", { path }),
  writeFile: (path: string, data: string) => runeCall<void>("fs.writeFile", { path, data }),
  exists: (path: string) => runeCall<boolean>("fs.exists", { path }),
};

// packages/api/src/process.ts
export const process = {
  exec: (cmd: string) => runeCall<ExecResult>("process.exec", { cmd }),
};
```

**Verification:** TypeScript compiles cleanly, types are accurate

---

### Task 4.2: CLI init template uses @rune/api
**Objective:** Default project template imports @rune/api and has working example
**Files:**
- Modify: `cli/src/templates/project/src/main.ts.tmpl`

**Template main.ts:**
```typescript
import { rune } from "@rune/api";

async function main() {
  const version = await rune.system.version();
  document.getElementById("version")!.textContent = `Rune ${version}`;
  
  const files = await rune.fs.readDir("~/Documents");
  console.log("Documents:", files);
}

main();
```

**Verification:** Init project, build, run — version displays in window

---

## Phase 5 — Plugin System

### Task 5.1: Plugin format + loader (C++)
**Objective:** Load `.so` plugins that register IPC handlers and UI extensions
**Files:**
- Create: `src/plugins/loader.hpp`
- Create: `src/plugins/loader.cpp`
- Create: `src/plugins/plugin.h` (public API header for plugin devs)

**Plugin API:**
```c
// Plugin must export:
extern "C" const char* rune_plugin_name();
extern "C" const char* rune_plugin_version();
extern "C" int rune_plugin_init(RuneAPI* api);
extern "C" void rune_plugin_deinit();
```

**Verification:** Create test plugin, load it, call its handlers from JS

---

### Task 5.2: `rune add` installs plugins
**Objective:** CLI downloads and installs plugins from a registry or local path
**Files:**
- Modify: `cli/src/commands/add.ts`

**Design:**
- `rune add ./my-plugin` — local .so
- `rune add @rune/fs-extra` — npm-scoped package with .so + TypeScript
- Stores in project's `plugins/` directory
- Updates `rune.config.json`

**Verification:** `rune add` local plugin, verify it loads at runtime

---

## Phase 6 — Dev Experience

### Task 6.1: `rune dev` — Hot reload server
**Objective:** Dev server that watches files, rebuilds, and reloads webview
**Files:**
- Modify: `cli/src/commands/dev.ts`
- Modify: Core — add `reload` IPC method

**Design:**
- CLI starts the C++ runtime with `--dev` flag
- Watches `src/` for changes (fs.watch)
- On change → sends `reload` IPC message to runtime
- Runtime reloads WebView

**Verification:** Change HTML, see window update without restart

---

### Task 6.2: `rune doctor` — Environment check
**Objective:** Verify all dependencies are installed
**Files:**
- Modify: `cli/src/commands/doctor.ts`

**Checks:**
- C++ compiler (g++/clang++)
- CMake >= 3.20
- WebKitGTK 4.1 dev headers
- GTK 3.0 dev headers
- nlohmann_json
- Node.js >= 18
- npm

**Verification:** Run on clean system, see what's missing

---

### Task 6.3: `rune build` — Production build
**Objective:** Bundle app for distribution
**Files:**
- Modify: `cli/src/commands/build.ts`

**Design:**
- Compiles C++ core in release mode
- Bundles TypeScript/HTML/CSS into `dist/`
- Creates self-contained directory or AppImage

**Verification:** `rune build && rune run`, app launches from built output

---

## Phase 7 — Documentation + Polish

### Task 7.1: README + CONTRIBUTING
**Files:** README.md, CONTRIBUTING.md

### Task 7.2: Example app
**Files:** examples/hello-rune/

### Task 7.3: Error handling hardening
- Graceful failure when WebKitGTK not available
- Meaningful error messages in CLI
- IPC timeout handling
- Invalid JSON handling

---

## Risks / Open Questions

1. **WebKitGTK 4.1 vs 6.0** — Which version to target? Arch/CachyOS ships 6.0, Ubuntu 24.04 ships 4.1. Start with 4.1 for compatibility, add 6.0 later.
2. **GTK thread model** — Single-thread with g_io_add_watch may have performance limits. Mitigation: move IPC to separate thread if needed (minor refactor).
3. **Wayland vs X11** — GTK3 handles both transparently. No extra work needed.
4. **Security** — Process execution and file system access need sandboxing consideration, but defer to post-MVP.
5. **Windows support** — Entirely different windowing backend (WebView2 + named pipes for IPC). Design with abstraction layer from start.
