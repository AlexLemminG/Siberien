#pragma once

#include "Component.h"
#include <vector>
#include <memory>
#include <algorithm>
#include "Math.h"
#include "bgfx/bgfx.h"
#include <assimp/scene.h>

class aiMesh;
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
	int randomColorTextureIdx = 0;

private:
	bool addedToRenderers = false;
};


class DirLight : public Component {
public:
	Color color = Colors::white;
	Vector3 dir = Vector3(0, -1, 0);
	void OnEnable() override {
		dirHandle = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);
		colorHandle = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4);

		bgfx::setUniform(dirHandle, &dir);
		bgfx::setUniform(colorHandle, &color);
	}
	void OnDisable() override {
		bgfx::destroy(dirHandle);
		bgfx::destroy(colorHandle);
	}

	bgfx::UniformHandle dirHandle = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle colorHandle = BGFX_INVALID_HANDLE;

	REFLECT_BEGIN(DirLight);
	REFLECT_VAR(color);
	REFLECT_VAR(dir);
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