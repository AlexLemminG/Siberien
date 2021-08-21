#pragma once
#include "Math.h"
#include "Component.h"

class aiAnimation;


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

	std::unordered_map<std::string, std::vector<AnimationKeyframe>> boneNameToKeyframesMapping;

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

	void OnEnable() override;

	void SetAnimation(std::shared_ptr<MeshAnimation> animation) { currentAnimation = animation; }

	REFLECT_BEGIN(Animator);
	REFLECT_VAR(speed);
	REFLECT_VAR(defaultAnimation);
	REFLECT_END();

private:
	void UpdateWorldMatrices();
};