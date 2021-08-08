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

	m_time = (currentTicks - m_startTicks) / ticksPerSecond;
	m_prevTicks = currentTicks;
	m_frameCount++;
}

void Time::Term() {}
