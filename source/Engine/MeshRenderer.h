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

namespace bimg {
	class ImageContainer;
}

class Mesh : public Object{
public:
	aiMesh* originalMeshPtr = nullptr;
	bgfx::VertexBufferHandle vertexBuffer = BGFX_INVALID_HANDLE;
	bgfx::IndexBufferHandle indexBuffer = BGFX_INVALID_HANDLE;
	std::vector<uint8_t> buffer;
	std::vector<uint16_t> indices;

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

class Shader : public Object {
public:
	bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
	~Shader();
	std::vector<std::shared_ptr<BinaryAsset>> buffers;
	REFLECT_BEGIN(Shader);
	REFLECT_END();
	std::string name;
};


class FullMeshAsset : public Object {
public:
	std::shared_ptr<const aiScene> scene;

	REFLECT_BEGIN(FullMeshAsset);
	REFLECT_END();
};
class Texture : public Object {
public:
	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
	bimg::ImageContainer* pImageContainer = nullptr;
	~Texture();
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
	std::shared_ptr<Texture> emissiveTex;
	std::vector<std::shared_ptr<Texture>> randomColorTextures;

	REFLECT_BEGIN(Material);
	REFLECT_VAR(colorTex);
	REFLECT_VAR(normalTex);
	REFLECT_VAR(emissiveTex);
	REFLECT_VAR(color);
	REFLECT_VAR(shader);
	REFLECT_VAR(randomColorTextures);
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

	void OnEnable() override;

	void OnDisable() override {
		enabledMeshRenderers.erase(std::find(enabledMeshRenderers.begin(), enabledMeshRenderers.end(), this));
	}

	REFLECT_BEGIN(MeshRenderer);
	REFLECT_VAR(mesh);
	REFLECT_VAR(material);
	REFLECT_END();

	std::vector<Matrix4> bonesFinalMatrices;
	std::vector<Matrix4> bonesWorldMatrices;
	std::vector<Matrix4> bonesLocalMatrices;
	int randomColorTextureIdx = 0;
};


class AnimationTransform {
public:
	Vector3 position = Vector3_zero;
	Quaternion rotation = Quaternion::identity;
	Vector3 scale = Vector3_one;

	static AnimationTransform Lerp(const AnimationTransform& a, const AnimationTransform& b, float t);

	void ToMatrix(Matrix4& matrix);
};
class MeshAnimation : public Object {
public:
	aiAnimation* assimAnimation = nullptr;
	AnimationTransform GetTransform(const std::string& bone, float t);

	REFLECT_BEGIN(MeshAnimation);
	REFLECT_END();
};

class Animator : public Component {
public:
	void Update() override;

	std::shared_ptr<MeshAnimation> currentAnimation;
	std::shared_ptr<MeshAnimation> defaultAnimation;
	float currentTime;
	float speed = 1.f;

	void OnEnable() override;

	void SetAnimation(std::shared_ptr<MeshAnimation> animation) { currentAnimation = animation; }

	REFLECT_BEGIN(Animator);
	REFLECT_VAR(speed);
	REFLECT_VAR(defaultAnimation);
	REFLECT_END();

private:
	void UpdateWorldMatrices();
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

class SphericalHarmonics : public Object {
public:
	std::vector<Color> coeffs;
	REFLECT_BEGIN(SphericalHarmonics);
	REFLECT_VAR(coeffs);
	REFLECT_END();
};

class Render;

class PostProcessingEffect : public Object {
public:
	void Draw(Render& render);

	float winScreenFade = 0.f;
	float intensityFromLastHit;
	float intensity;
private:
	std::shared_ptr<Shader> shader;

	REFLECT_BEGIN(PostProcessingEffect);
	REFLECT_VAR(shader);
	REFLECT_END();

	static std::vector<std::shared_ptr<PostProcessingEffect>> activeEffects; //TODO no static please
	void ScreenSpaceQuad(
		float _textureWidth,
		float _textureHeight,
		float _texelHalf,
		bool _originBottomLeft);
};