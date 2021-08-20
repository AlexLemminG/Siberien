#include "bx/allocator.h"

#include "MeshRenderer.h"
#include "bgfx/bgfx.h"
#include <windows.h>
#include "bimg/bimg.h"
#include "bimg/decode.h"
#include "Time.h"
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
DECLARE_TEXT_ASSET(DirLight);
DECLARE_TEXT_ASSET(PointLight);

std::vector<MeshRenderer*> MeshRenderer::enabledMeshRenderers;

void PointLight::OnEnable() {
	pointLights.push_back(this);
}

void PointLight::OnDisable() {
	pointLights.erase(std::find(pointLights.begin(), pointLights.end(), this));
}
std::vector<PointLight*> PointLight::pointLights;

Vector3 aiConvert(const aiVector3D& v) {
	return Vector3(v.x, v.y, v.z);
}
Quaternion aiConvert(const aiQuaternion& v) {
	return Quaternion(v.x, v.y, v.z, v.w);
}

template<typename OUT_T, typename IN_T>
static bool GetAnimValue(OUT_T& out, IN_T* keys, int numKeys, float t) {
	if (numKeys <= 0) {
		return false;
	}
	float tPos = Mathf::Repeat(t * 1000.f, keys[numKeys - 1].mTime);

	for (int iPos = 0; iPos < numKeys; iPos++) {
		if (iPos == numKeys - 1 || keys[iPos + 1].mTime >= tPos) {
			auto thisKey = keys[iPos];
			auto nextKey = iPos == numKeys - 1 ? thisKey : keys[iPos + 1];
			tPos = (tPos - thisKey.mTime) / Mathf::Max(nextKey.mTime - thisKey.mTime, 0.001f);

			out = Mathf::Lerp(aiConvert(thisKey.mValue), aiConvert(nextKey.mValue), tPos);
			return true;
		}
	}
	return false;
}
void AnimationTransform::ToMatrix(Matrix4& matrix) {
	matrix = Matrix4::Transform(position, rotation.ToMatrix(), scale);
}

AnimationTransform MeshAnimation::GetTransform(const std::string& bone, float t) {
	for (int iNode = 0; iNode < assimAnimation->mNumChannels; iNode++) {
		auto* channel = assimAnimation->mChannels[iNode];
		if (bone != channel->mNodeName.C_Str()) {
			continue;
		}

		AnimationTransform transform;
		GetAnimValue(transform.position, channel->mPositionKeys, channel->mNumPositionKeys, t);
		GetAnimValue(transform.rotation, channel->mRotationKeys, channel->mNumRotationKeys, t);
		GetAnimValue(transform.scale, channel->mScalingKeys, channel->mNumScalingKeys, t);

		return transform;
	}
	ASSERT(false);
	return AnimationTransform();
}

AnimationTransform AnimationTransform::Lerp(const AnimationTransform& a, const AnimationTransform& b, float t) {
	AnimationTransform l;
	l.position = Mathf::Lerp(a.position, b.position, t);
	l.rotation = Mathf::Lerp(a.rotation, b.rotation, t);
	l.scale = Mathf::Lerp(a.scale, b.scale, t);
	return l;
}

void Animator::Update() {
	auto meshRenderer = gameObject()->GetComponent<MeshRenderer>();
	if (!meshRenderer) {
		return;
	}
	auto mesh = meshRenderer->mesh;
	if (!mesh) {
		return;
	}

	if (currentAnimation != nullptr) {
		currentTime += Time::deltaTime() * speed;
		for (auto& bone : mesh->bones) {
			auto transform = currentAnimation->GetTransform(bone.name, currentTime);
			transform.rotation = bone.inverseTPoseRotation * transform.rotation;
			transform.ToMatrix(meshRenderer->bonesLocalMatrices[bone.idx]);

			//SetRot(meshRenderer->bonesLocalMatrices[bone.idx], GetRot(bone.initialLocal));//TODO f this shit
		}
	}
	UpdateWorldMatrices();


	for (auto bone : meshRenderer->bonesWorldMatrices) {
		//Dbg::Draw(bone, 5);
	}
}

void Animator::OnEnable() {
	auto meshRenderer = gameObject()->GetComponent<MeshRenderer>();
	if (!meshRenderer) {
		return;
	}
	auto mesh = meshRenderer->mesh;
	if (!mesh) {
		return;
	}

	for (auto& bone : mesh->bones) {
		meshRenderer->bonesLocalMatrices[bone.idx] = bone.initialLocal;
	}
	UpdateWorldMatrices();

	currentAnimation = defaultAnimation;
}

void Animator::UpdateWorldMatrices() {
	auto meshRenderer = gameObject()->GetComponent<MeshRenderer>();
	if (!meshRenderer) {
		return;
	}
	auto mesh = meshRenderer->mesh;
	if (!mesh) {
		return;
	}

	for (auto& bone : mesh->bones) {
		if (bone.parentBoneIdx != -1) {
			auto& parentBoneMatrix = meshRenderer->bonesWorldMatrices[bone.parentBoneIdx];
			meshRenderer->bonesWorldMatrices[bone.idx] = parentBoneMatrix * meshRenderer->bonesLocalMatrices[bone.idx];
		}
		else {
			auto& parentBoneMatrix = gameObject()->transform()->matrix;
			meshRenderer->bonesWorldMatrices[bone.idx] = parentBoneMatrix * meshRenderer->bonesLocalMatrices[bone.idx];
		}
		meshRenderer->bonesFinalMatrices[bone.idx] = meshRenderer->bonesWorldMatrices[bone.idx] * bone.offset;
	}
}


DECLARE_TEXT_ASSET(Animator);
DECLARE_TEXT_ASSET(MeshAnimation);

void MeshRenderer::OnEnable() {
	this->worldMatrix = Matrix4::Identity();
	enabledMeshRenderers.push_back(this);
	if (mesh && mesh->bones.size() > 0) {
		bonesWorldMatrices.resize(mesh->bones.size());
		bonesLocalMatrices.resize(mesh->bones.size());
		bonesFinalMatrices.resize(mesh->bones.size());
	}
	if (material && material->randomColorTextures.size() > 0) {
		randomColorTextureIdx = Random::Range(0, material->randomColorTextures.size());
	}
}
