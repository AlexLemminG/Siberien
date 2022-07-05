#pragma once

#include <vector>
#include "SDL_scancode.h"
#include "SMath.h"
#include "System.h"

class SE_CPP_API Input : public System<Input> {
public:
	virtual bool Init() override;
	virtual void Term() override;
	static void Update();


	static bool GetKeyDown(SDL_Scancode code);
	static bool GetKey(SDL_Scancode code);
	static bool GetKeyUp(SDL_Scancode code);

	bool GetKeyDown(const std::string& code) const;
	bool GetKey(const std::string& code) const;
	bool GetKeyUp(const std::string& code) const;

	static bool GetMouseButton(int button);
	static bool GetMouseButtonDown(int button);
	static float GetMouseScrollY();

	static Vector2 GetMousePosition();
	static Vector2 GetMouseDeltaPosition();

	static bool GetQuit();

	REFLECT_BEGIN(Input);
	REFLECT_METHOD_EXPLICIT("GetKeyDown", static_cast<bool(Input::*)(const std::string&) const>(&Input::GetKeyDown));
	REFLECT_METHOD_EXPLICIT("GetKeyUp", static_cast<bool(Input::*)(const std::string&) const>(&Input::GetKeyUp));
	REFLECT_METHOD_EXPLICIT("GetKey", static_cast<bool(Input::*)(const std::string&) const>(&Input::GetKey));
	REFLECT_END()
};