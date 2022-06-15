#pragma once

#include "System.h"
#include "GameEvents.h"//TODO rename GameEvent

class Render;
class ICamera;

class SE_CPP_API RenderEvents : public System<RenderEvents> {
public:
	//TODO dont pass Render
	GameEvent<Render&> onSceneRendered;
	GameEvent<const ICamera&> onBeforeCameraRender;
};