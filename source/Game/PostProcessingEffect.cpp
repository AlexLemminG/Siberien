#include "PostProcessingEffect.h"
#include "Serialization.h"
#include "Common.h"
#include "Graphics.h"
#include "Material.h"
#include "Engine.h"

class Render;

void PostProcessingEffect::OnEnable() {
	handle = RenderEvents::Get()->onSceneRendered.Subscribe([this](Render& render) {Draw(render); });
	material = Object::Instantiate(material);
}

void PostProcessingEffect::OnDisable() {
	RenderEvents::Get()->onSceneRendered.Unsubscribe(handle);
}

void PostProcessingEffect::Draw(Render& render) {
	OPTICK_EVENT();
	if (!material) {
		return;
	}
	Vector4& params = material->vectors[0].value;
	params[0] = intensity;
	params[1] = intensityFromLastHit;
	params[2] = winScreenFade;
	Graphics::Get()->Blit(material, 1);
}
DECLARE_TEXT_ASSET(PostProcessingEffect);