#include "Animation.h"
#include "assimp/anim.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "STime.h"
#include "Mesh.h"
#include "Common.h"


Vector3 aiConvert(const aiVector3D& v) {
	return Vector3(v.x, v.y, v.z);
}
Quaternion aiConvert(const aiQuaternion& v) {
	return Quaternion(v.w, v.x, v.y, v.z);
}

template<typename OUT_T, typename IN_T>
static bool GetAnimValue(OUT_T& out, IN_T* keys, int numKeys, float t) {
	if (numKeys <= 0) {
		return false;
	}
	float tPos = Mathf::Repeat(t, keys[numKeys - 1].mTime);

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


int MeshAnimation::GetBoneIdx(const std::string& bone) {
	auto it = std::find(channelNames.begin(), channelNames.end(), bone);
	if (it != channelNames.end()) {
		return it - channelNames.begin();
	}
	return -1;
}

AnimationTransform MeshAnimation::GetTransform(const std::string& bone, float t) {
	auto idx = GetBoneIdx(bone);
	if (idx != -1) {
		return GetTransform(idx, t);
	}
	ASSERT(false);
	return AnimationTransform{};//TODO just return false
}

AnimationTransform MeshAnimation::GetTransform(int channelIdx, float t) {
	auto& channel = channelKeyframes[channelIdx];
	t = Mathf::Repeat(t, channel[channel.size() - 1].time);

	auto lowerBound = std::lower_bound(channel.begin(), channel.end(), t, [](const auto& keyframe, float time) {return keyframe.time < time; });
	int iBefore = lowerBound - channel.begin();
	if (iBefore > 0) {
		iBefore--;
	}
	if (lowerBound == channel.end() || iBefore == channel.size() - 1) {
		return channel[channel.size() - 1].transform;
	}
	else {
		float lerpT = Mathf::InverseLerp(channel[iBefore].time, channel[iBefore + 1].time, t);
		return AnimationTransform::Lerp(channel[iBefore].transform, channel[iBefore + 1].transform, lerpT);
	}
}

void MeshAnimation::DeserializeFromAssimp(aiAnimation* anim) {
	const std::string armaturePrefix = "Armature|";
	std::string animName = anim->mName.C_Str();
	if (animName.find(armaturePrefix.c_str()) == 0) {
		animName = animName.substr(armaturePrefix.size(), animName.size() - armaturePrefix.size());
	}
	name = animName;

	for (int iChannel = 0; iChannel < anim->mNumChannels; iChannel++) {
		auto channel = anim->mChannels[iChannel];
		std::vector<double> times;
		for (int i = 0; i < channel->mNumPositionKeys; i++) {
			times.push_back(channel->mPositionKeys[i].mTime);
		}
		for (int i = 0; i < channel->mNumRotationKeys; i++) {
			times.push_back(channel->mRotationKeys[i].mTime);
		}
		for (int i = 0; i < channel->mNumScalingKeys; i++) {
			times.push_back(channel->mScalingKeys[i].mTime);
		}
		std::sort(times.begin(), times.end());
		times.erase(std::unique(times.begin(), times.end()), times.end());

		channelNames.push_back(channel->mNodeName.C_Str());
		auto& keyframes = channelKeyframes.emplace_back();

		for (auto t : times) {
			//TODO probably has errors
			AnimationTransform transform;
			GetAnimValue(transform.position, channel->mPositionKeys, channel->mNumPositionKeys, t);
			GetAnimValue(transform.rotation, channel->mRotationKeys, channel->mNumRotationKeys, t);
			GetAnimValue(transform.scale, channel->mScalingKeys, channel->mNumScalingKeys, t);

			float time = t / 1000.f;
			keyframes.push_back(AnimationKeyframe{ time, transform });
		}
	}
}

AnimationTransform AnimationTransform::Lerp(const AnimationTransform& a, const AnimationTransform& b, float t) {
	AnimationTransform l;
	l.position = Mathf::Lerp(a.position, b.position, t);
	l.rotation = Mathf::Lerp(a.rotation, b.rotation, t);
	l.scale = Mathf::Lerp(a.scale, b.scale, t);
	return l;
}

void Animator::Update() {
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
			transform.ToMatrix(meshRenderer->bonesLocalMatrices[bone.idx]);
		}
	}
	UpdateWorldMatrices();
}

void Animator::OnEnable() {
	meshRenderer = gameObject()->GetComponent<MeshRenderer>();
	transform = gameObject()->transform();
	if (meshRenderer == nullptr) {
		return;
	}
	auto mesh = meshRenderer->mesh;
	if (!mesh) {
		return;
	}

	//TODO support disable in middle of enable
	transform = gameObject()->transform();

	for (const auto& bone : mesh->bones) {
		meshRenderer->bonesLocalMatrices[bone.idx] = bone.initialLocal;
	}
	UpdateWorldMatrices();


	currentAnimation = defaultAnimation;
}

void Animator::UpdateWorldMatrices() {
	if (meshRenderer == nullptr) {
		return;
	}
	auto mesh = meshRenderer->mesh;
	if (!mesh) {
		return;
	}

	for (auto& bone : mesh->bones) {
		if (bone.parentBoneIdx != -1) {
			const auto& parentBoneMatrix = meshRenderer->bonesWorldMatrices[bone.parentBoneIdx];
			meshRenderer->bonesWorldMatrices[bone.idx] = parentBoneMatrix * meshRenderer->bonesLocalMatrices[bone.idx];
		}
		else {
			const auto& parentBoneMatrix = transform->matrix;
			meshRenderer->bonesWorldMatrices[bone.idx] = parentBoneMatrix * meshRenderer->bonesLocalMatrices[bone.idx];
		}
		meshRenderer->bonesFinalMatrices[bone.idx] = meshRenderer->bonesWorldMatrices[bone.idx] * bone.offset;
	}
}


DECLARE_TEXT_ASSET(Animator);
DECLARE_TEXT_ASSET(MeshAnimation);
