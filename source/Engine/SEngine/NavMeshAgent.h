#pragma once

#include "Component.h"
#include "SMath.h"

class NavMeshAgent : public Component {
public:
	void SetDestination(const Vector3& destination);
	virtual void Update() override;
private:
	bool hasDestination = false;
	Vector3 destination;

	std::vector<Vector3> currentPath;

	REFLECT_BEGIN(NavMeshAgent);
	REFLECT_END();

};