#pragma once

#include "System.h"
#include <string>

class Cmd : public System<Cmd> {
public:
	class Handler {
	public:
		virtual void Handle(std::string command, const std::vector<std::string>& args) = 0;
	};

	void ProcessCommands(int argc, char** args);

	void AddHandler(std::string command, std::shared_ptr<Handler> handler);

private:
	void ExecuteCommands(int first, int last, const std::vector<std::string>& strs);

	std::unordered_map<std::string, std::shared_ptr<Handler>> handlers;
};