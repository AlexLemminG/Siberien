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


void MeshRenderer::OnEnable() {
	addedToRenderers = material != nullptr && mesh != nullptr;
	if (addedToRenderers) {
		enabledMeshRenderers.push_back(this);
	}
	if (mesh && mesh->bones.size() > 0) {
		bonesWorldMatrices.resize(mesh->bones.size());
		bonesLocalMatrices.resize(mesh->bones.size());
		bonesFinalMatrices.resize(mesh->bones.size());
	}
	if (material && material->randomColorTextures.size() > 0) {
		randomColorTextureIdx = Random::Range(0, material->randomColorTextures.size());
	}
	m_transform = gameObject()->transform().get();
}

void MeshRenderer::OnDisable() {
	if (addedToRenderers) {
		enabledMeshRenderers.erase(std::find(enabledMeshRenderers.begin(), enabledMeshRenderers.end(), this));
		addedToRenderers = false;
	}
}
std::vector<MeshRenderer*> MeshRenderer::enabledMeshRenderers;
