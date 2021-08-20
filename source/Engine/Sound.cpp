#include "Sound.h"
#include "Resources.h"
#include "Config.h"

class AudioClipImporter : public AssetImporter {
	virtual std::shared_ptr<Object> Import(AssetDatabase& database, const std::string& path) override
	{
		return nullptr;
	}
};


AudioClip::AudioClip() {}
AudioClip::~AudioClip() {}


DECLARE_BINARY_ASSET(AudioClip, AudioClipImporter);
REGISTER_SYSTEM(AudioSystem);


void AudioSystem::Play(std::shared_ptr<AudioClip> clip) {
	if (!CfgGetBool("soundEnabled")) {
		return;
	}
}
