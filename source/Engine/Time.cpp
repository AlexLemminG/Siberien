#include "Math.h"
#include "Time.h"
#include "SDL.h"
#include "Config.h"

REGISTER_SYSTEM(Time);

bool Time::Init() {
	m_maxDeltaTime = CfgGetFloat("maximumDeltaTime");//TODO use
	m_fixedDeltaTime = CfgGetFloat("fixedDeltaTime");

	m_prevTicks = SDL_GetTicks();
	m_startTicks = m_prevTicks;

	return true;
}

void Time::Update() {
	uint32_t currentTicks = SDL_GetTicks();
	float ticksPerSecond = 1000.f;

	m_deltaTime = (currentTicks - m_prevTicks) / ticksPerSecond;

	m_deltaTime = Mathf::Min(m_deltaTime, m_maxDeltaTime);

	m_time += m_deltaTime;

	m_prevTicks = currentTicks;
	m_frameCount++;
}

void Time::Term() {}
