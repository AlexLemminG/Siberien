#pragma once

#include "Object.h"
#include "bgfx/bgfx.h"//TODO forward declare
#include <vector>
#include "SMath.h"

class aiMesh;
class aiScene;
class MeshAnimation;
class Mesh;

//TODO rename ?
struct FullMeshAsset_Node {
	std::shared_ptr<Mesh> mesh;
	Matrix4 localTransformMatrix;
	std::vector<FullMeshAsset_Node> childNodes;
};
class FullMeshAsset : public Object {
public:

	FullMeshAsset_Node rootNode;

	std::vector<std::shared_ptr<Mesh>> meshes;
	std::vector<std::shared_ptr<MeshAnimation>> animations;

	REFLECT_BEGIN(FullMeshAsset);
	REFLECT_END();
};

struct RawVertexData {
	static constexpr int bonesPerVertex = 4;

	Vector3 pos;
	Vector3 normal;
	Vector3 tangent;
	Vector2 uv;
	float boneWeights[bonesPerVertex];
	uint8_t boneIndices[bonesPerVertex];
};

class MeshPhysicsData;


class Mesh : public Object {
public:
	class BoneInfo {
	public:
		std::string name = "";
		int idx = 0;
		int parentBoneIdx = -1;
		Matrix4 offset;
		Matrix4 initialLocal;
	};

	bgfx::VertexBufferHandle vertexBuffer = BGFX_INVALID_HANDLE;
	bgfx::IndexBufferHandle indexBuffer = BGFX_INVALID_HANDLE;

	std::string name;

	std::string assetMaterialName;

	std::vector<RawVertexData> rawVertices;
	std::vector<uint16_t> rawIndices;

	std::vector<BoneInfo> bones;

	//TODO unique_ptr
	std::shared_ptr<MeshPhysicsData> physicsData;

	AABB aabb;
	Sphere boundingSphere;

	~Mesh();


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

	std::vector<uint8_t> CreateBuffer(const std::vector<RawVertexData>& rawVertices) const;
};