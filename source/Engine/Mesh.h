#pragma once

#include "Object.h"
#include "bgfx/bgfx.h"//TODO forward declare
#include <vector>
#include "Math.h"

class aiMesh;
class aiScene;
class MeshAnimation;

//TODO rename ?
class FullMeshAsset : public Object {
public:
	std::shared_ptr<const aiScene> scene;

	REFLECT_BEGIN(FullMeshAsset);
	REFLECT_END();
};

class Mesh : public Object {
public:
	std::string name;
	aiMesh* originalMeshPtr = nullptr;
	bgfx::VertexBufferHandle vertexBuffer = BGFX_INVALID_HANDLE;
	bgfx::IndexBufferHandle indexBuffer = BGFX_INVALID_HANDLE;

	static constexpr int bonesPerVertex = 4;
	Matrix4 globalInverseTransform;

	~Mesh();

	class BoneInfo {
	public:
		std::string name;
		int idx = 0;
		int parentBoneIdx = -1;
		Matrix4 offset;
		Matrix4 initialLocal;
		Quaternion inverseTPoseRotation;
	};

	std::vector<BoneInfo> bones;
	std::shared_ptr<MeshAnimation> tPoseAnim;

	void Init();
private:
	void FillBoneParentInfo();

	REFLECT_BEGIN(Mesh);
	REFLECT_END();
};

class MeshVertexLayout {
public:
	bgfx::VertexLayout bgfxLayout;
	std::vector<bgfx::Attrib::Enum> attributes;

	MeshVertexLayout& Begin();

	MeshVertexLayout& AddPosition();

	MeshVertexLayout& AddNormal();

	MeshVertexLayout& AddTexCoord();

	MeshVertexLayout& AddTangent();

	MeshVertexLayout& AddIndices();

	MeshVertexLayout& AddWeights();

	MeshVertexLayout& End();

	std::vector<uint8_t> CreateBuffer(aiMesh* mesh) const;

	static std::vector<uint16_t> CreateIndices(aiMesh* mesh);
};