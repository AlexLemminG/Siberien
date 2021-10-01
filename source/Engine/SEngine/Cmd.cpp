#include "Cmd.h"
#include <SDL.h>

//TODO cleaner cmd conventions design



void Cmd::ProcessCommands(int argc, char** args) {
	std::vector<std::string> strArgs;
	for (int i = 0; i < argc; i++) {
		strArgs.push_back(args[i]);
	}

	int startIdx = -1;
	for (int i = 0; i < strArgs.size(); i++) {
		const auto& cmd = strArgs[i];
		if (cmd.size() >= 2 && cmd.find("--") == 0) {
			if (startIdx != -1) {
				ExecuteCommands(startIdx, i - 1, strArgs);
			}
			startIdx = i;
		}
	}
	if (startIdx != -1) {
		ExecuteCommands(startIdx, strArgs.size() - 1, strArgs);
	}
}

void Cmd::AddHandler(std::string command, std::shared_ptr<Handler> handler) {
	//TODO duplicates check
	handlers[command] = handler;
}

void Cmd::ExecuteCommands(int first, int last, const std::vector<std::string>& strs) {
	std::string command = strs[first].substr(2, strs[first].size()-2);
	if (command == "quit") {
		Engine::Get()->Quit();
		return;
	}

	std::vector<std::string> args;
	for (int i = first + 1; i <= last; i++) {
		args.push_back(strs[i]);
	}

	auto itHandler = handlers.find(command);
	if (itHandler != handlers.end()) {
		itHandler->second->Handle(command, args);
	}
	else {
		//TODO unknown command
	}
}

REGISTER_SYSTEM(Cmd);