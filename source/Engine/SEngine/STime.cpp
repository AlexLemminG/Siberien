

#include "SMath.h"
#include "STime.h"
#include "SDL.h"
#include "Config.h"
#include "Resources.h"
#include "LuaSystem.h"

REGISTER_SYSTEM(Time);

static float s_deltaTime = 0.f;
static float s_realTime = 0.f;
static float s_time = 0.f;
static int s_frameCount = 0;
static float s_fixedDeltaTime = 1.f / 60.f;
static float s_maxDeltaTime = 0.1f;
static float s_timeScale = 1.f;

class TimeSettings : public Object {
public:
	float maximumDeltaTime = 0.1f;
	float fixedDeltaTime = 1.f / 60.f;
	float timeScale = 1.f;

	REFLECT_BEGIN(TimeSettings);
	REFLECT_VAR(maximumDeltaTime);
	REFLECT_VAR(fixedDeltaTime);
	REFLECT_VAR(timeScale);
	REFLECT_END();
};

DECLARE_TEXT_ASSET(TimeSettings);


bool Time::Init() {
	//TODO some default way loading of system settings
	settings = AssetDatabase::Get()->Load<TimeSettings>("settings.asset");
	if (settings == nullptr) {
		settings = std::make_shared<TimeSettings>();
	}

	s_deltaTime = 0.f;
	s_realTime = 0.f;
	s_time = 0.f;
	s_frameCount = 0;

	s_maxDeltaTime = settings->maximumDeltaTime;
	s_fixedDeltaTime = settings->fixedDeltaTime;
	s_timeScale = settings->timeScale;

	m_prevTicks = SDL_GetTicks();
	m_startTicks = m_prevTicks;

	LuaSystem::Get()->RegisterFunction("deltaTime", &deltaTime);
	LuaSystem::Get()->RegisterFunction("time", &time);

	return true;
}

void Time::Update() {
	uint32_t currentTicks = SDL_GetTicks();
	float ticksPerSecond = 1000.f;

	s_maxDeltaTime = settings->maximumDeltaTime;
	s_fixedDeltaTime = settings->fixedDeltaTime;
	s_timeScale = settings->timeScale;

	s_deltaTime = (currentTicks - m_prevTicks) / ticksPerSecond;

	s_deltaTime = Mathf::Min(s_deltaTime, s_maxDeltaTime) * s_timeScale;

	s_realTime = (currentTicks - m_startTicks) / ticksPerSecond;

	s_time += s_deltaTime;

	m_prevTicks = currentTicks;
	s_frameCount++;
}

void Time::Term() {}

float Time::deltaTime() { return s_deltaTime; }

float Time::getRealTime() { return s_realTime; }

float Time::time() { return s_time; }

int Time::frameCount() { return s_frameCount; }

float Time::fixedDeltaTime() { return s_fixedDeltaTime; }
