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

	void OnEnable() override;

	void OnDisable() override;

	REFLECT_BEGIN(MeshRenderer);
	REFLECT_VAR(mesh);
	REFLECT_VAR(material);
	REFLECT_END();

	std::vector<Matrix4> bonesFinalMatrices;
	std::vector<Matrix4> bonesWorldMatrices;
	std::vector<Matrix4> bonesLocalMatrices;
	Transform* m_transform = nullptr;
	int randomColorTextureIdx = 0;//TODO no sins

private:
	bool addedToRenderers = false;
};


class DirLight : public Component {
public:
	Color color = Colors::white;

	void OnEnable();
	void OnDisable();

	static std::vector<DirLight*> dirLights;

	REFLECT_BEGIN(DirLight);
	REFLECT_VAR(color);
	REFLECT_END();
};

class PointLight : public Component {
public:
	void OnEnable();
	void OnDisable();

	static std::vector<PointLight*> pointLights;

	Color color = Colors::white;
	float radius = 5.f;
	float innerRadius = 4.f;

	REFLECT_BEGIN(PointLight);
	REFLECT_VAR(color);
	REFLECT_VAR(radius);
	REFLECT_VAR(innerRadius);
	REFLECT_END();
};