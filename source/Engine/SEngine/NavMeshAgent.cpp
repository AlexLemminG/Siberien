#include "NavMeshAgent.h"
#include "Navigation.h"
#include "GameObject.h"
#include "Transform.h"
#include "Physics.h"
#include "Dbg.h"
#include <DetourNavMeshQuery.h>
#include <DetourCommon.h>
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
inline bool inRange(const float* v1, const float* v2, const float r, const float h)
{
	const float dx = v2[0] - v1[0];
	const float dy = v2[1] - v1[1];
	const float dz = v2[2] - v1[2];
	return (dx * dx + dz * dz) < r * r && fabsf(dy) < h;
}
static bool getSteerTarget(dtNavMeshQuery* navQuery, const float* startPos, const float* endPos,
	const float minTargetDist,
	const dtPolyRef* path, const int pathSize,
	float* steerPos, unsigned char& steerPosFlag, dtPolyRef& steerPosRef,
	float* outPoints = 0, int* outPointCount = 0)
{
	// Find steer target.
	static const int MAX_STEER_POINTS = 30;
	float steerPath[MAX_STEER_POINTS * 3];
	unsigned char steerPathFlags[MAX_STEER_POINTS];
	dtPolyRef steerPathPolys[MAX_STEER_POINTS];
	int nsteerPath = 0;
	navQuery->findStraightPath(startPos, endPos, path, pathSize,
		steerPath, steerPathFlags, steerPathPolys, &nsteerPath, MAX_STEER_POINTS);
	if (!nsteerPath)
		return false;

	if (outPoints && outPointCount)
	{
		*outPointCount = nsteerPath;
		for (int i = 0; i < nsteerPath; ++i)
			dtVcopy(&outPoints[i * 3], &steerPath[i * 3]);
	}


	// Find vertex far enough to steer to.
	int ns = 0;
	while (ns < nsteerPath)
	{
		// Stop at Off-Mesh link or when point is further than slop away.
		if ((steerPathFlags[ns] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
			!inRange(&steerPath[ns * 3], startPos, minTargetDist, 1000.0f))
			break;
		ns++;
	}
	// Failed to find good point to steer to.
	if (ns >= nsteerPath)
		return false;

	dtVcopy(steerPos, &steerPath[ns * 3]);
	steerPos[1] = startPos[1];
	steerPosFlag = steerPathFlags[ns];
	steerPosRef = steerPathPolys[ns];

	return true;
}

std::vector<Vector3> FindPath(Vector3 from, Vector3 to) {
	int m_pathIterNum = 0;
	dtPolyRef m_startRef;
	dtPolyRef m_endRef;
	Vector3 m_spos_vec;
	Vector3 m_epos_vec;
	dtQueryFilter m_filter;
	constexpr int MAX_POLYS = 1000;
	constexpr int MAX_SMOOTH = 1000;

	dtPolyRef m_polys[MAX_POLYS];
	int m_npolys;
	auto m_navQuery = NavMesh::Get()->GetQuery();

	FindClosestPoint(from, m_spos_vec, m_startRef);
	FindClosestPoint(to, m_epos_vec, m_endRef);
	auto m_spos = &m_spos_vec.x;
	auto m_epos = &m_epos_vec.x;
	int m_nsmoothPath = 0;
	float m_smoothPath[MAX_SMOOTH * 3];
	std::vector<Vector3> result;
	result.push_back(m_spos_vec);
	if (m_startRef && m_endRef)
	{
		m_navQuery->findPath(m_startRef, m_endRef, m_spos, m_epos, &m_filter, m_polys, &m_npolys, MAX_POLYS);

		m_nsmoothPath = 0;

		if (m_npolys)
		{
			// Iterate over the path to find smooth path on the detail mesh surface.
			dtPolyRef polys[MAX_POLYS];
			memcpy(polys, m_polys, sizeof(dtPolyRef) * m_npolys);
			int npolys = m_npolys;

			float iterPos[3], targetPos[3];
			m_navQuery->closestPointOnPoly(m_startRef, m_spos, iterPos, 0);
			m_navQuery->closestPointOnPoly(polys[npolys - 1], m_epos, targetPos, 0);

			static const int MAX_STEER_POINTS = 300;
			float steerPath[MAX_STEER_POINTS * 3];
			unsigned char steerPathFlags[MAX_STEER_POINTS];
			dtPolyRef steerPathPolys[MAX_STEER_POINTS];
			int nsteerPath = 0;
			auto res = m_navQuery->findStraightPath(iterPos, targetPos, polys, npolys,
				steerPath, steerPathFlags, steerPathPolys, &nsteerPath, MAX_STEER_POINTS);
			if (res == DT_SUCCESS) {
				for (int iPoly = 0; iPoly < nsteerPath; iPoly++) {
					result.push_back(Vector3(&steerPath[iPoly * 3]));
				}
			}
		}
	}


	return result;
}

void NavMeshAgent::SetDestination(const Vector3& destination) {
	if (FindClosestPoint(destination, this->destination)) {
		hasDestination = true;
	}
}
void NavMeshAgent::Update() {
	if (Input::GetMouseButton(0)) {
		auto ray = Camera::GetMain()->ScreenPointToRay(Input::GetMousePosition());
		Physics::RaycastHit hit;
		if (Physics::Raycast(hit, ray, 1000.f)) {
			currentPath.clear();
			SetDestination(hit.GetPoint());
		}
	}
	if (hasDestination) {
		auto currentPos = gameObject()->transform()->GetPosition();
		if (currentPath.size() == 0) {
			currentPath = FindPath(currentPos, destination);
		}
		auto& path = currentPath;
		if (path.size() > 0) {
			Dbg::DrawLine(currentPos, path[0]);
		}

		for (int i = 0; i < ((int)path.size()) - 1; i++) {
			Dbg::DrawLine(path[i], path[i + 1]);
		}

		if (path.size() > 0) {
			float threshold = 0.1f;
			Vector3 targetPos = path[path.size() - 1];
			for (int i = 0; i < path.size(); i++) {
				auto p = path[i];
				if (Vector3::Distance(p, currentPos) > threshold) {
					targetPos = p;
					break;
				}
				else {
					i--;
					path.erase(path.begin());
				}
			}
			float speed = 5.f;
			auto dir = targetPos - currentPos;
			dir.y = 0.f;
			//TODO fix LookRotation to accept parrallel stuff
			if (dir.Length() > Mathf::epsilon) {
				gameObject()->transform()->SetRotation(LookRotation(dir.Normalized(), Vector3_up));
			}
			currentPos = Mathf::MoveTowards(currentPos, targetPos, Time::deltaTime() * speed);
			gameObject()->transform()->SetPosition(currentPos);
		}
	}


}
