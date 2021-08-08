#include "Time.h"
#include "SDL.h"

REGISTER_SYSTEM(Time);

bool Time::Init() {
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
