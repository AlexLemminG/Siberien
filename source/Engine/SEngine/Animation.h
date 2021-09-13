#pragma once
#include "SMath.h"
#include "Component.h"

class aiAnimation;
class Transform;
class MeshRenderer;

class AnimationTransform {
public:
	Vector3 position = Vector3_zero;
	Quaternion rotation = Quaternion::identity;
	Vector3 scale = Vector3_one;

	static AnimationTransform Lerp(const AnimationTransform& a, const AnimationTransform& b, float t);

	void ToMatrix(Matrix4& matrix);
};

class AnimationKeyframe {
public:
	float time;
	AnimationTransform transform;
};

class MeshAnimation : public Object {
public:
	std::string name;
	AnimationTransform GetTransform(const std::string& bone, float t);
	AnimationTransform GetTransform(int channelIdx, float t);

	int GetBoneIdx(const std::string& bone);

	std::vector<std::string> channelNames;
	std::vector<std::vector<AnimationKeyframe>> channelKeyframes;

	void DeserializeFromAssimp(aiAnimation* anim);

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

	std::shared_ptr<MeshRenderer> meshRenderer;
	std::shared_ptr<Transform> transform;

	void OnEnable() override;

	void SetAnimation(std::shared_ptr<MeshAnimation> animation) { currentAnimation = animation; }

	REFLECT_BEGIN(Animator);
	REFLECT_VAR(speed);
	REFLECT_VAR(defaultAnimation);
	REFLECT_END();

private:
	void UpdateWorldMatrices();
};