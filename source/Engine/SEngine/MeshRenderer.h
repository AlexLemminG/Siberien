#pragma once

#include "Component.h"
#include <vector>
#include <memory>
#include <algorithm>
#include "SMath.h"
#include "bgfx/bgfx.h"

struct aiMesh;
class aiScene;
class aiNode;
class MeshAnimation;
class Material;
class Mesh;

namespace bimg {
	class ImageContainer;
}
class Transform;
class SE_CPP_API MeshRendererAbstract : public Component {
public:
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;

	virtual void OnEnable() override;
	virtual void OnDisable() override;

	REFLECT_BEGIN(MeshRendererAbstract, Component);
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

protected:
	bool castsShadows = true;
	bool addedToRenderers = false;
};

class SE_CPP_API MeshRenderer : public MeshRendererAbstract {
public:

	static std::vector<MeshRendererAbstract*> enabledMeshRenderers;
	static std::vector<MeshRendererAbstract*> enabledShadowCasters;

	virtual void OnEnable() override;
	virtual void OnDisable() override;

	REFLECT_BEGIN(MeshRenderer, MeshRendererAbstract);
	REFLECT_END();
};