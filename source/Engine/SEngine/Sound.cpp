#include "Common.h"
#include "Defines.h"
#include "Sound.h"
#include "Resources.h"
#include "Config.h"

REGISTER_SYSTEM(AudioSystem);


void AudioSystem::Play(std::shared_ptr<AudioClip> clip) {
	if (!CfgGetBool("soundEnabled")) {
		return;
	}
}

AudioClip::AudioClip() {}

AudioClip::~AudioClip() {}
