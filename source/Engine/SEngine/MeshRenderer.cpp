

#include "bx/allocator.h"

#include "MeshRenderer.h"
#include "bgfx/bgfx.h"
#include <windows.h>
#include "bimg/bimg.h"
#include "bimg/decode.h"
#include "STime.h"
#include "GameObject.h"
#include "Dbg.h"
#include "Render.h"
#include "Material.h"
#include "Mesh.h"
#include "assimp/scene.h"
//#include "bgfx_utils.h"


void PrintHierarchy(aiNode* node, int offset) {
	std::string off = "";
	for (int i = 0; i < offset; i++) {
		off += " ";
	}
	LogError(off + node->mName.C_Str());
	for (int i = 0; i < node->mNumChildren; i++) {
		PrintHierarchy(node->mChildren[i], offset + 1);
	}
}

DECLARE_TEXT_ASSET(MeshRenderer);


void MeshRendererAbstract::OnEnable() {
	addedToRenderers = material != nullptr && mesh != nullptr;
	if (addedToRenderers) {
		MeshRenderer::enabledMeshRenderers.push_back(this);
		if (castsShadows) {
			MeshRenderer::enabledShadowCasters.push_back(this);
		}
	}
	if (mesh && mesh->bones.size() > 0) {
		bonesWorldMatrices.resize(mesh->bones.size());
		bonesLocalMatrices.resize(mesh->bones.size());
		bonesFinalMatrices.resize(mesh->bones.size());
	}
	if (material && material->randomColorTextures.size() > 0) {
		randomColorTextureIdx = Random::Range(0, material->randomColorTextures.size());
	}
}

void MeshRendererAbstract::OnDisable() {
	if (addedToRenderers) {
		auto it = std::find(MeshRenderer::enabledMeshRenderers.begin(), MeshRenderer::enabledMeshRenderers.end(), this);
		MeshRenderer::enabledMeshRenderers[it - MeshRenderer::enabledMeshRenderers.begin()] = MeshRenderer::enabledMeshRenderers.back();
		MeshRenderer::enabledMeshRenderers.pop_back();
		if (castsShadows) {
			auto it2 = std::find(MeshRenderer::enabledShadowCasters.begin(), MeshRenderer::enabledShadowCasters.end(), this);
			MeshRenderer::enabledShadowCasters[it2 - MeshRenderer::enabledShadowCasters.begin()] = MeshRenderer::enabledShadowCasters.back();
			MeshRenderer::enabledShadowCasters.pop_back();
		}
		addedToRenderers = false;
	}
}
void MeshRenderer::OnEnable() {
	MeshRendererAbstract::OnEnable();
	m_transform = gameObject()->transform().get();
}

void MeshRenderer::OnDisable() {
	MeshRendererAbstract::OnDisable();
}
std::vector<MeshRendererAbstract*> MeshRenderer::enabledMeshRenderers;
std::vector<MeshRendererAbstract*> MeshRenderer::enabledShadowCasters;
