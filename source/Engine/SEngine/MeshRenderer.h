#pragma once

#include "Component.h"
#include <vector>
#include <memory>
#include <algorithm>
#include "SMath.h"
#include "bgfx/bgfx.h"
#include <assimp/scene.h>

struct aiMesh;
//class aiScene;
class MeshAnimation;
class Material;
class Mesh;

namespace bimg {
	class ImageContainer;
}
class Transform;
class MeshRenderer : public Component {
public:
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
	static std::vector<MeshRenderer*> enabledMeshRenderers;
	static std::vector<MeshRenderer*> enabledShadowCasters;

	void OnEnable() override;
	void OnDisable() override;

	REFLECT_BEGIN(MeshRenderer, Component);
	REFLECT_ATTRIBUTE(ExecuteInEditModeAttribute());
	REFLECT_VAR(mesh);
	REFLECT_VAR(material);
	REFLECT_VAR(castsShadows);
	REFLECT_END();

	std::vector<Matrix4> bonesFinalMatrices;
	std::vector<Matrix4> bonesWorldMatrices;
	std::vector<Matrix4> bonesLocalMatrices;
	Transform* m_transform = nullptr;
	int randomColorTextureIdx = 0;//TODO no sins

private:
	bool castsShadows = true;
	bool addedToRenderers = false;
};