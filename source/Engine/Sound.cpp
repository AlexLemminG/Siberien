#include "Sound.h"
#include "Resources.h"
#include "SDL_mixer.h"
#include "Config.h"

class AudioClipImporter : public AssetImporter {

	virtual std::shared_ptr<Object> Import(AssetDatabase& database, const std::string& path) override
	{
		auto fullPath = database.GetAssetPath(path);
		auto bin = database.LoadBinaryAsset(path);

		auto wav = Mix_LoadWAV(fullPath.c_str());

		if (wav == nullptr) {
			return nullptr;
		}

		auto audio = std::make_shared<AudioClip>();
		wav->volume /= 2;
		audio->wav = wav;

		database.AddAsset(audio, path, "");

		return audio;
	}
};


AudioClip::AudioClip() {}
AudioClip::~AudioClip() {
	if (wav) {
		Mix_FreeChunk(wav);
		wav = nullptr;
	}
}


DECLARE_BINARY_ASSET(AudioClip, AudioClipImporter);
REGISTER_SYSTEM(AudioSystem);


void AudioSystem::Play(std::shared_ptr<AudioClip> clip) {
	if (!clip || !clip->wav || !CfgGetBool("soundEnabled")) {
		return;
	}

	Mix_PlayChannel(-1, clip->wav, 0);
}
