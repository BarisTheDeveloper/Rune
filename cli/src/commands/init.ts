import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const TEMPLATE_DIR = path.join(__dirname, "..", "templates", "project");

export async function runInit(projectName?: string): Promise<void> {
  const name = projectName || "my-rune-app";
  const targetDir = path.resolve(name);

  if (fs.existsSync(targetDir)) {
    console.log(`Directory '${name}' already exists.`);
    return;
  }

  console.log(`Creating Rune project: ${name}`);

  // Copy template files
  copyTemplateDir(TEMPLATE_DIR, targetDir, {
    PROJECT_NAME: name,
  });

  // Create empty scripts directory
  fs.mkdirSync(path.join(targetDir, "scripts"), { recursive: true });

  // Copy run-dev.sh
  const devScriptSrc = path.join(__dirname, "..", "templates", "scripts", "run-dev.sh");
  const devScriptDst = path.join(targetDir, "scripts", "run-dev.sh");
  fs.copyFileSync(devScriptSrc, devScriptDst);
  fs.chmodSync(devScriptDst, 0o755);

  console.log("");
  console.log(`  cd ${name}`);
  console.log("  rune dev");
  console.log("");
}

function copyTemplateDir(
  srcDir: string,
  dstDir: string,
  vars: Record<string, string>
): void {
  const entries = fs.readdirSync(srcDir, { withFileTypes: true });

  for (const entry of entries) {
    const srcPath = path.join(srcDir, entry.name);

    // Remove .tmpl extension for destination
    let dstName = entry.name;
    if (dstName.endsWith(".tmpl")) {
      dstName = dstName.slice(0, -5);
    }

    const dstPath = path.join(dstDir, dstName);

    if (entry.isDirectory()) {
      fs.mkdirSync(dstPath, { recursive: true });
      copyTemplateDir(srcPath, dstPath, vars);
    } else {
      let content = fs.readFileSync(srcPath, "utf-8");
      // Replace template variables
      for (const [key, value] of Object.entries(vars)) {
        content = content.replaceAll(`{{${key}}}`, value);
      }
      fs.mkdirSync(path.dirname(dstPath), { recursive: true });
      fs.writeFileSync(dstPath, content);
    }
  }
}
