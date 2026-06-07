import { spawn, execSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import os from "node:os";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));

function findRuneCore(): string {
  const candidates = [
    // Installed globally by install.sh
    path.join(os.homedir(), ".rune", "rune-core"),
    path.join(os.homedir(), ".local", "bin", "rune-core"),
    // Bundled with Rune source repo (development)
    path.resolve(__dirname, "..", "..", "..", "build", "src", "rune-core"),
  ];

  for (const candidate of candidates) {
    if (fs.existsSync(candidate)) {
      return candidate;
    }
  }

  return "";
}

export async function runDev(): Promise<void> {
  const cwd = process.cwd();
  let runeCore = findRuneCore();

  if (!runeCore) {
    // Try to build from source (Rune repo development)
    const repoRoot = path.resolve(__dirname, "..", "..", "..");
    if (fs.existsSync(path.join(repoRoot, "CMakeLists.txt"))) {
      console.log("Building Rune core from source...");
      try {
        execSync(
          "mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && make -j$(nproc)",
          { cwd: repoRoot, stdio: "inherit" }
        );
        runeCore = path.join(repoRoot, "build", "src", "rune-core");
      } catch {
        console.error("Error: Failed to build Rune core.");
        process.exit(1);
      }
    }
  }

  if (!runeCore || !fs.existsSync(runeCore)) {
    console.error("Error: rune-core not found.");
    console.error("  Build it: cd Rune && mkdir -p build && cd build && cmake .. && make");
    console.error("  Or install: cd Rune/cli && bash install.sh");
    process.exit(1);
  }

  console.log(`[RUNE] Starting dev server...`);
  console.log(`  Runtime: ${runeCore}`);
  console.log(`  App dir: ${cwd}`);
  console.log("");

  const child = spawn(runeCore, ["--app-dir", cwd], {
    stdio: "inherit",
    env: { ...process.env, GDK_BACKEND: "x11" },
  });

  child.on("exit", (code) => {
    process.exit(code ?? 0);
  });
}
