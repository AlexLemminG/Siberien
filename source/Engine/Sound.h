#pragma once

#include "Object.h"
#include "Serialization.h"
#include "System.h"


class AudioClip : public Object {
public:
	AudioClip();
	~AudioClip();

	REFLECT_BEGIN(AudioClip);
	REFLECT_END()
};


class AudioSystem : public System<AudioSystem> {
public:
	void Play(std::shared_ptr<AudioClip> clip);
};