import { execSync } from "node:child_process";

export async function runDoctor(): Promise<void> {
  console.log("Rune Doctor");
  console.log("===========\n");

  const checks: [string, string][] = [
    ["Node.js", "node --version"],
    ["yarn", "yarn --version"],
    ["g++", "g++ --version 2>&1 | head -1"],
    ["cmake", "cmake --version 2>&1 | head -1"],
    ["pkg-config", "pkg-config --version"],
    ["webkit2gtk-4.1", "pkg-config --modversion webkit2gtk-4.1 2>/dev/null || echo 'NOT FOUND'"],
    ["gtk+-3.0", "pkg-config --modversion gtk+-3.0 2>/dev/null || echo 'NOT FOUND'"],
    ["nlohmann_json", "pkg-config --modversion nlohmann_json 2>/dev/null || echo 'NOT FOUND'"],
  ];

  for (const [name, cmd] of checks) {
    try {
      const out = execSync(cmd, { encoding: "utf-8", timeout: 5000 }).trim().split("\n")[0];
      const ok = !out?.includes("NOT FOUND") && out !== "";
      console.log(`  ${ok ? "✅" : "❌"} ${name.padEnd(18)} ${out}`);
    } catch {
      console.log(`  ❌ ${name.padEnd(18)} NOT FOUND`);
    }
  }

  console.log("");
}
