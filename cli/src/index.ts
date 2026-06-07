import { Command } from "commander";
import { runInit } from "./commands/init.js";
import { runDev } from "./commands/dev.js";
import { runBuild } from "./commands/build.js";
import { runRun } from "./commands/run.js";
import { runDoctor } from "./commands/doctor.js";
import { runAdd } from "./commands/add.js";

const app = new Command();

app
  .name("rune")
  .description("Build native. Stay fast.");

// rune init [name]
app
  .command("init [name]")
  .description("Creates a Rune Project")
  .action(async (name?: string) => {
    await runInit(name);
  });

// rune dev
app
  .command("dev")
  .description("Runs the project you've created with rune init")
  .action(async () => {
    await runDev();
  });

// rune build
app
  .command("build")
  .description("Build application for distribution")
  .action(async () => {
    await runBuild();
  });

// rune run
app
  .command("run")
  .description("Run the built application")
  .action(async () => {
    await runRun();
  });

// rune doctor
app
  .command("doctor")
  .description("Check environment for required dependencies")
  .action(async () => {
    await runDoctor();
  });

// rune add <plugin>
app
  .command("add <plugin>")
  .description("Add a Rune plugin")
  .action(async (plugin: string) => {
    await runAdd(plugin);
  });

app.parse();
