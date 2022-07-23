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
	bool GetTransform(AnimationTransform& outTransform, const std::string& bone, float t);
	void GetTransform(AnimationTransform& outTransform, int channelIdx, float t);

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
	void ResetTime() { currentTime = 0.f; }

	REFLECT_BEGIN(Animator);
	REFLECT_ATTRIBUTE(ExecuteInEditModeAttribute());
	REFLECT_VAR(speed);
	REFLECT_VAR(defaultAnimation);
	REFLECT_METHOD(SetAnimation);
	REFLECT_METHOD(ResetTime);
	REFLECT_END();

private:
	void UpdateWorldMatrices();
};