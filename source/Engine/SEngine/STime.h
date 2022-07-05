#pragma once

#include "System.h"
#include "Defines.h"

class TimeSettings;

class SE_CPP_API Time : public System<Time> {
public:
	bool Init() override;
	void Update() override;
	void Term() override;

	//TODO slow?
	static float deltaTime();
	static float getRealTime();
	static float time();
	static int frameCount();
	static float fixedDeltaTime();
private:
	std::shared_ptr<TimeSettings> settings;
	uint32_t m_prevTicks = 0;
	uint32_t m_startTicks = 0;

	REFLECT_BEGIN(Time);
	REFLECT_METHOD(time);
	REFLECT_METHOD(deltaTime);
	REFLECT_END();
};