module;
#include <print>
#include <fstream>
export module cmd_ide;
import context;

void create_vs_tasks();
void create_vscode_tasks();

export bool cmd_ide(context&, std::string_view args) {
    if (args.empty() || args == "?") {
        std::println("Usage: gbs ide=[arg]");
        std::println("arg:");
        std::println("  vs       Creates 'tasks.vs.json' for Visual Studio");
        std::println("  vscode   Creates 'tasks.vs.json' for Visual Studio Code");
        return true;
	}
	if (args == "vs") {
		create_vs_tasks();
		return true;
	}
	else if (args == "vscode") {
        create_vscode_tasks();
        return true;
    }

	return false;
}

void create_vs_tasks() {
	std::string_view const tasks = R"({
  "version": "0.2.1",
  "outDir": "\"${workspaceRoot}\\gbs.out\"",
  "tasks": [
    {
      "taskLabel": "Enumerate compilers",
      "appliesTo": "*/",
      "type": "launch",
      "contextType": "custom",
      "workingDirectory": "${file}",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${env.GBS_BIN}'",
        "-ArgumentList",
        "'enum_cl'"
      ]
    },
    {
      "taskLabel": "Build",
      "appliesTo": "*/",
      "type": "launch",
      "contextType": "build",
      "workingDirectory": "${file}",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${env.GBS_BIN}'",
        "-ArgumentList",
        "'cl=msvc config=release,warnings build'"
      ]
    },
    {
      "taskLabel": "Rebuild",
      "appliesTo": "*/",
      "type": "launch",
      "contextType": "rebuild",
      "workingDirectory": "${file}",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${env.GBS_BIN}'",
        "-ArgumentList",
        "'clean cl=msvc config=release,warnings build'"
      ]
    },
    {
      "taskLabel": "Clean",
      "appliesTo": "*/",
      "type": "launch",
      "contextType": "clean",
      "workingDirectory": "${file}",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${env.GBS_BIN}'",
        "-ArgumentList",
        "clean"
      ]
    },
    {
      "taskLabel": "build [msvc]",
      "appliesTo": "*/",
      "type": "launch",
      "contextType": "custom",
      "workingDirectory": "${file}",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${env.GBS_BIN}'",
        "-ArgumentList",
        "'cl=msvc build'"
      ]
    },
    {
      "taskLabel": "build [clang]",
      "appliesTo": "*/",
      "type": "launch",
      "contextType": "custom",
      "workingDirectory": "${file}",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${env.GBS_BIN}'",
        "-ArgumentList",
        "'cl=clang build'"
      ]
    },
    {
      "taskLabel": "build [gcc]",
      "appliesTo": "*/",
      "type": "launch",
      "contextType": "custom",
      "workingDirectory": "${file}",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${env.GBS_BIN}'",
        "-ArgumentList",
        "'cl=gcc build'"
      ]
    },
    {
      "taskLabel": "download [clang]",
      "appliesTo": "*/",
      "type": "launch",
      "contextType": "custom",
      "workingDirectory": "${file}",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${env.GBS_BIN}'",
        "-ArgumentList",
        "get_cl=clang"
      ]
    },
    {
      "taskLabel": "download [gcc]",
      "appliesTo": "*/",
      "type": "launch",
      "contextType": "custom",
      "workingDirectory": "${file}",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${env.GBS_BIN}'",
        "-ArgumentList",
        "get_cl=gcc"
      ]
    },
    {
      "taskLabel": "analyze [msvc]",
      "appliesTo": "*/",
      "type": "launch",
      "contextType": "custom",
      "workingDirectory": "${file}",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${env.GBS_BIN}'",
        "-ArgumentList",
        "'cl=msvc config=analyze,debug,warnings build'"
      ]
    },
    {
      "taskLabel": "analyze [clang]",
      "appliesTo": "*/",
      "type": "launch",
      "contextType": "custom",
      "workingDirectory": "${file}",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${env.GBS_BIN}'",
        "-ArgumentList",
        "'cl=clang config=analyze,debug,warnings build'"
      ]
    }
  ]
})";
	std::ofstream tasks_file("tasks.vs.json");
    tasks_file << tasks;
}

void create_vscode_tasks() {
    std::string_view const tasks = R"({
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build",
            "type": "shell",
            "command": "${env:GBS_BIN} build=release,warnings",
            "problemMatcher": [],
            "group": {
                "kind": "build",
                "isDefault": true
            }
        },
        {
            "label": "rebuild",
            "type": "shell",
            "command": "${env:GBS_BIN} clean build=release,warnings"
        },
        {
            "label": "clean",
            "type": "shell",
            "command": "${env:GBS_BIN} clean"
        },
        {
            "label": "test",
            "type": "shell",
            "command": "${env:GBS_BIN} test",
            "problemMatcher": [],
            "group": {
                "kind": "test",
                "isDefault": true
            }
        }
    ]
})";
	std::ofstream tasks_file("tasks.vs.json");
    tasks_file << tasks;
}