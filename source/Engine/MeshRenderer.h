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

class Mesh : public Object{
public:
	aiMesh* originalMeshPtr = nullptr;
	bgfx::VertexBufferHandle vertexBuffer{};
	bgfx::IndexBufferHandle indexBuffer{};
	std::vector<uint8_t> buffer;
	std::vector<uint16_t> indices;
private:
	REFLECT_BEGIN(Mesh);
	REFLECT_END();
};

class Shader : public Object {
public:
	bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
	std::vector<std::shared_ptr<BinaryAsset>> buffers;
	REFLECT_BEGIN(Shader);
	REFLECT_END();
};


class FullMeshAsset : public Object {
public:
	std::shared_ptr<const aiScene> scene;

	REFLECT_BEGIN(FullMeshAsset);
	REFLECT_END();
};
class Texture : public Object {
public:
	bgfx::TextureHandle handle;
	REFLECT_BEGIN(Texture);
	REFLECT_END();

	std::shared_ptr<BinaryAsset> bin;
};
class Material : public Object {
public:
	Color color = Colors::white;
	std::shared_ptr<Shader> shader;
	std::shared_ptr<Texture> colorTex;
	std::shared_ptr<Texture> normalTex;

	REFLECT_BEGIN(Material);
	REFLECT_VAR(colorTex);
	REFLECT_VAR(normalTex);
	REFLECT_VAR(color);
	REFLECT_VAR(shader);
	REFLECT_END();
};

void InitVertexLayouts();

class MeshRenderer : public Component {
public:
	//bx::
	Matrix4 worldMatrix;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
	static std::vector<MeshRenderer*> enabledMeshRenderers;

	void OnEnable() override {
		this->worldMatrix = Matrix4::Identity();
		enabledMeshRenderers.push_back(this);
	}
	void OnDisable() override {
		enabledMeshRenderers.erase(std::find(enabledMeshRenderers.begin(), enabledMeshRenderers.end(), this), enabledMeshRenderers.end());
	}

	REFLECT_BEGIN(MeshRenderer);
	REFLECT_VAR(mesh);
	REFLECT_VAR(material);
	REFLECT_END();
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

	bgfx::UniformHandle dirHandle;
	bgfx::UniformHandle colorHandle;

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