#pragma once

#include "Component.h"
#include <vector>
#include <memory>
#include <algorithm>
#include "Math.h"

class aiMesh;

class Mesh : public Object{
public:
	aiMesh* originalMeshPtr = nullptr;
private:
	REFLECT_BEGIN(Mesh);
	REFLECT_END();
};

class Material : public Object {
	Color color;

	REFLECT_BEGIN(Material);
	REFLECT_VAR(color, Colors::white);
	REFLECT_END();
};

class Shader : public Object {

};

class MeshRenderer : public Component {
public:
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
	static std::vector<MeshRenderer*> enabledMeshRenderers;

	void OnEnable() override {
		enabledMeshRenderers.push_back(this);
	}
	void OnDisable() override {
		enabledMeshRenderers.erase(std::find(enabledMeshRenderers.begin(), enabledMeshRenderers.end(), this), enabledMeshRenderers.end());
	}

	REFLECT_BEGIN(MeshRenderer);
	REFLECT_VAR(mesh, nullptr);
	REFLECT_VAR(material, nullptr);
	REFLECT_END();
};