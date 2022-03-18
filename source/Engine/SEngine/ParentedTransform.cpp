#include "ParentedTransform.h"

#include "MeshRenderer.h"
#include "GameObject.h"

DECLARE_TEXT_ASSET(ParentedTransform);

void ParentedTransform::Update() {
	//TODO call order
	UpdateMatrix();
}

void ParentedTransform::UpdateMatrix() {
	//TODO ensure prev matrix is not changed before setting new one and assert
	//TODO optimize
	if (parentTransform != nullptr) {
		gameObject()->transform()->SetMatrix(parentTransform->GetMatrix() * localMatrix);
		return;
	}
	if (parentMeshRenderer != nullptr) {
		gameObject()->transform()->SetMatrix(parentMeshRenderer->bonesWorldMatrices[parentMeshRendererBoneIdx] * localMatrix);
		return;
	}
}

void ParentedTransform::SetParent(const std::shared_ptr<Transform>& parent) {
	this->parentTransform = parent;
	parentMeshRenderer = nullptr;
	UpdateMatrix();
}

void ParentedTransform::SetParent(const std::shared_ptr<MeshRenderer>& parent, int boneIdx) {
	this->parentTransform = nullptr;
	parentMeshRenderer = parent;
	parentMeshRendererBoneIdx = boneIdx;
	UpdateMatrix();
}
