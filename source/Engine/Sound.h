#pragma once

#include "Object.h"
#include "Serialization.h"
#include "System.h"


class Mix_Chunk;


class AudioClip : public Object {
public:
	AudioClip();
	~AudioClip();

	Mix_Chunk* wav = nullptr;

	REFLECT_BEGIN(AudioClip);
	REFLECT_END()
};


class AudioSystem : public System<AudioSystem> {
public:
	void Play(std::shared_ptr<AudioClip> clip);
};