export module cmd_ide;
import context;
import std;

void create_vs_tasks();
void create_vscode_tasks();

export bool cmd_ide(context&, std::string_view args) {
    if (args.empty() || args == "?") {
        std::println("Usage: gbs ide=[arg]");
        std::println("arg:");
        std::println("  vs       Creates 'tasks.vs.json' for Visual Studio");
        std::println("  vscode   Create Visual Studio Code tasks");
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
  "tasks": [
    {
      "taskLabel": "Build",
      "appliesTo": "/",
      "type": "default",
      "contextType": "build",
      "workingDirectory": "${workspaceRoot}/",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${GBS_BIN}'",
        "-ArgumentList",
        "'build=debug,warnings'"
      ]
    },
    {
      "taskLabel": "Rebuild",
      "appliesTo": "/",
      "type": "default",
      "contextType": "rebuild",
      "workingDirectory": "${workspaceRoot}/",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${GBS_BIN}'",
        "-ArgumentList",
        "'clean build=debug,warnings'"
      ]
    },
    {
      "taskLabel": "Clean",
      "appliesTo": "/",
      "type": "default",
      "contextType": "clean",
      "workingDirectory": "${workspaceRoot}/",
      "command": "pwsh",
      "args": [
        "-Command Start-Process",
        "-Wait",
        "-UseNewEnvironment",
        "-NoNewWindow",
        "-FilePath",
        "'${GBS_BIN}'",
        "-ArgumentList",
        "clean"
      ]
    }
  ]
})";
	std::ofstream tasks_file("tasks.vs.json");
    tasks_file << tasks;
    return;
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
    return;
}