#include "NavMeshAgent.h"
#include "Navigation.h"
#include "GameObject.h"
#include "Transform.h"
#include "Physics.h"
#include "Dbg.h"
#include <DetourNavMeshQuery.h>
#include <DetourCommon.h>
#include <DetourCrowd.h>
#include "Input.h"
#include "Camera.h"
#include "STime.h"

DECLARE_TEXT_ASSET(NavMeshAgent);

bool FindClosestPoint(Vector3 from, Vector3& result, dtPolyRef& poly) {
	poly = dtPolyRef();
	Vector3 maxDistance = Vector3_one * 1.f;
	Vector3 pointOnNavmesh;
	dtQueryFilter filter;
	auto status = NavMesh::Get()->GetQuery()->findNearestPoly(&from.x, &maxDistance.x, &filter, &poly, &pointOnNavmesh.x);
	if (status == DT_SUCCESS && poly) {
		result = pointOnNavmesh;
		return true;
	}
	else {
		return false;
		//TODO move to closest point anyway?
	}
}

bool FindClosestPoint(Vector3 from, Vector3& result) {
	dtPolyRef poly;
	return FindClosestPoint(from, result, poly);
}

void NavMeshAgent::SetDestination(const Vector3& destination) {
	dtPolyRef poly;
	if (FindClosestPoint(destination, this->destination, poly)) {
		hasDestination = true;
		NavMesh::Get()->GetCrowd()->requestMoveTarget(agentId, poly, &this->destination.x);
		//NavMesh::Get()->GetCrowd()->getEditableAgent(agentId)->targetReplan = true;
	}
}
void NavMeshAgent::SetVelocity(const Vector3& velocity) {
	auto realVelocity = Mathf::ClampLength(velocity, maxSpeed);
	NavMesh::Get()->GetCrowd()->requestMoveVelocity(agentId, &realVelocity.x);
	hasDestination = false;
}

void NavMeshAgent::Update() {
	UpdatePosition();
	UpdateRotation();
}
void NavMeshAgent::UpdatePosition() {
	auto* agent = NavMesh::Get()->GetCrowd()->getAgent(agentId);
	gameObject()->transform()->SetPosition(Vector3(agent->npos));
}

void NavMeshAgent::UpdateRotation() {
	auto* agent = NavMesh::Get()->GetCrowd()->getAgent(agentId);
	auto dirXZ = Vector3(agent->vel).xz();
	if (dirXZ.LengthSquared() > Mathf::epsilon) {
		float angle = Mathf::SignedAngle(Vector2(1, 0), dirXZ);
		float deltaAngle = Mathf::DeltaAngle(currentAngleXZ, angle);
		if (Mathf::Abs(deltaAngle) > Mathf::epsilon) {
			float lerpMulti = 1.f;
			lerpMulti *= Mathf::Min(1.f, Mathf::Abs(deltaAngle) * 1.f);
			currentAngleXZ = Mathf::MoveTowardsAngle(currentAngleXZ, angle, Mathf::DegToRad(maxAngularSpeed * lerpMulti) * Time::deltaTime());
			Vector3 dir = Vector3(Mathf::Cos(currentAngleXZ), 0.f, Mathf::Sin(currentAngleXZ));
			gameObject()->transform()->SetRotation(LookRotation(dir, Vector3_up));
		}
	}
}

void NavMeshAgent::OnEnable() {
	auto pos = gameObject()->transform()->GetPosition();
	dtCrowdAgentParams params{};
	params.radius = 0.616f;
	params.maxSpeed = maxSpeed;
	params.maxAcceleration = maxAcceleration;
	params.height = 2.f;
	params.collisionQueryRange = 5.f;
	params.pathOptimizationRange = 5.f;
	//TODO vector const
	currentAngleXZ = Mathf::SignedAngle(Vector2(1, 0), gameObject()->transform()->GetForward().xz());
	agentId = NavMesh::Get()->GetCrowd()->addAgent(&pos.x, &params);
}
void NavMeshAgent::OnDisable() {
	NavMesh::Get()->GetCrowd()->removeAgent(agentId);
}
