#pragma once

#include "Transform.h"
#include "Reflect.h"

class Transform;
class MeshRenderer;

class SE_CPP_API ParentedTransform : public Component {
public:
	void Update();

	void SetParent(const std::shared_ptr<Transform>& parent);
	void SetParent(const std::shared_ptr<MeshRenderer>& parent, int boneIdx);

private:
	std::shared_ptr<Transform> parentTransform;
	std::shared_ptr<MeshRenderer> parentMeshRenderer;
	int parentMeshRendererBoneIdx = 0;
	Matrix4 localMatrix = Matrix4::Identity();

	void UpdateMatrix();

	REFLECT_BEGIN(ParentedTransform);
	REFLECT_VAR(localMatrix);
	REFLECT_END();
};