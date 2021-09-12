#pragma once
#include "System.h"
#include <vector>
#include <functional>

class GameEventHandle {
public:
	int id = -1;
};

template<typename... Args>
class GameEvent {
public:
	typedef std::function<void(Args...)> HANDLER_TYPE;

	GameEventHandle Subscribe(HANDLER_TYPE handler) {
		GameEventHandle handle{ nextHandleIdx++ };
		handlers.push_back({ handle.id, handler });
		return handle;
	}
	void Unsubscribe(GameEventHandle handle) {
		auto l = [handle](const HandleWithHandler& x) { return x.handleId == handle.id; };
		auto it = std::find_if(handlers.begin(), handlers.end(), l);
		if (it != handlers.end()) {
			handlers.erase(it);
		}
	}
	void Invoke(Args... args) {
		for (HandleWithHandler& handler : handlers) {
			handler.handler(args...);
		}
	}
	void UnsubscribeAll() {
		handlers.clear();
	}

private:
	int nextHandleIdx = 0;
	class HandleWithHandler {
	public:
		HandleWithHandler(int handleId, HANDLER_TYPE& handler) :handleId(handleId), handler(handler) {}
		int handleId = 0;
		HANDLER_TYPE handler;
	};
	std::vector<HandleWithHandler> handlers;
};