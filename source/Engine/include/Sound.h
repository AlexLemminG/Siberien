#pragma once

#include "Defines.h"
#include "Object.h"
#include "Serialization.h"
#include "System.h"


class SE_CPP_API AudioClip : public Object {
public:
	AudioClip();
	~AudioClip();

	REFLECT_BEGIN(AudioClip);
	REFLECT_END()
};


class SE_CPP_API AudioSystem : public System<AudioSystem> {
public:
	void Play(std::shared_ptr<AudioClip> clip);
};