#include "PostProcessingEffect.h"
#include "bgfx/bgfx.h"
#include "Render.h"
#include "Texture.h"
#include "System.h"
#include "Shader.h"
#include "Resources.h"
#include "Serialization.h"
#include "Graphics.h"
#include "Material.h"


void PostProcessingEffect::OnEnable() {
	handle = RenderEvents::Get()->onSceneRendered.Subscribe([this](Render& render) {Draw(render); });
	material = Object::Instantiate(material);
}

void PostProcessingEffect::OnDisable() {
	RenderEvents::Get()->onSceneRendered.Unsubscribe(handle);
}

void PostProcessingEffect::Draw(Render& render) {
	OPTICK_EVENT();
	
	Vector4& params = material->vectors[0].value;
	params[0] = intensity;
	params[1] = intensityFromLastHit;
	params[2] = winScreenFade;
	Graphics::Get()->Blit(material);
}
DECLARE_TEXT_ASSET(PostProcessingEffect);