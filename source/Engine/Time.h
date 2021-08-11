#pragma once

#include "System.h"

class Time : public System<Time>{
public:
	bool Init() override;
	void Update() override;
	void Term() override;

	//TODO slow?
	static float deltaTime() { return Get()->m_deltaTime; }
	static float getRealTime() { return Get()->m_realTime; }
	static float time() { return Get()->m_time; }
	static float frameCount() { return Get()->m_frameCount; }
	static float fixedDeltaTime() { return Get()->m_fixedDeltaTime; }
private:

	float m_fixedDeltaTime = 0.f;
	float m_deltaTime = 0.f;
	float m_realTime = 0.f;
	float m_time = 0.f;
	float m_timeScale = 1.0f;
	int m_frameCount = 0;

	uint32_t m_prevTicks = 0;
	uint32_t m_startTicks = 0;

	float m_maxDeltaTime = 0.1;
};