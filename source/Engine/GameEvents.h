#pragma once
#include "System.h"
#include <vector>
#include <functional>


template<typename... Args>
class GameEvent {
public:
	typedef std::function<void(Args...)> HANDLER_TYPE;

	int Subscribe(HANDLER_TYPE handler) {
		int handle = nextHandleIdx++;
		handlers.push_back({ handle, handler });
		return handle;
	}
	void Unsubscribe(int handle) {
		auto l = [handle](const HandlerWithHandler& x) { return x.handle == handle; };
		handlers.erase(std::remove_if(handlers.begin(), handlers.end(), l));
	}
	void Invoke(Args... args) {
		for (HandlerWithHandler& handler : handlers) {
			handler.handler(args...);
		}
	}

private:
	int nextHandleIdx = 0;
	class HandlerWithHandler {
	public:
		HandlerWithHandler(int handle, HANDLER_TYPE& handler) :handle(handle), handler(handler){}
		int handle = 0;
		HANDLER_TYPE handler;
	};
	std::vector<HandlerWithHandler> handlers;
};

class EnemyCreepController;
class GameEvents : public System<GameEvents> {
	bool Init() override;

public:
	GameEvent<EnemyCreepController*> creepDeath;
};