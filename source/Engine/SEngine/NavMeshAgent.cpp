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
	static const int MAX_STEER_POINTS = 3;
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
#ifdef DUMP_REQS
		printf("pi  %f %f %f  %f %f %f  0x%x 0x%x\n",
			m_spos[0], m_spos[1], m_spos[2], m_epos[0], m_epos[1], m_epos[2],
			m_filter.getIncludeFlags(), m_filter.getExcludeFlags());
#endif

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

			for (int iPoly = 0; iPoly < m_npolys; iPoly++) {
				Vector3 pos;
				m_navQuery->closestPointOnPoly(m_polys[iPoly], m_epos, &pos.x, nullptr);
				result.push_back(pos);
			}

			static const float STEP_SIZE = 0.5f;
			static const float SLOP = 0.01f;

			m_nsmoothPath = 0;

			dtVcopy(&m_smoothPath[m_nsmoothPath * 3], iterPos);
			m_nsmoothPath++;

			// Move towards target a small advancement at a time until target reached or
			// when ran out of memory to store the path.
			while (npolys && m_nsmoothPath < MAX_SMOOTH)
			{
				// Find location to steer towards.
				float steerPos[3];
				unsigned char steerPosFlag;
				dtPolyRef steerPosRef;

				if (!getSteerTarget(m_navQuery, iterPos, targetPos, SLOP,
					polys, npolys, steerPos, steerPosFlag, steerPosRef))
					break;

				bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) ? true : false;
				bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ? true : false;

				// Find movement delta.
				float delta[3], len;
				dtVsub(delta, steerPos, iterPos);
				len = dtMathSqrtf(dtVdot(delta, delta));
				// If the steer target is end of path or off-mesh link, do not move past the location.
				if ((endOfPath || offMeshConnection) && len < STEP_SIZE)
					len = 1;
				else
					len = STEP_SIZE / len;
				float moveTgt[3];
				dtVmad(moveTgt, iterPos, delta, len);

				// Move
				float result[3];
				dtPolyRef visited[16];
				int nvisited = 0;
				m_navQuery->moveAlongSurface(polys[0], iterPos, moveTgt, &m_filter,
					result, visited, &nvisited, 16);

				//npolys = fixupCorridor(polys, npolys, MAX_POLYS, visited, nvisited);
				//npolys = fixupShortcuts(polys, npolys, m_navQuery);

				float h = 0;
				m_navQuery->getPolyHeight(polys[0], result, &h);
				result[1] = h;
				dtVcopy(iterPos, result);

				// Handle end of path and off-mesh links when close enough.
				if (endOfPath && inRange(iterPos, steerPos, SLOP, 1.0f))
				{
					// Reached end of path.
					dtVcopy(iterPos, targetPos);
					if (m_nsmoothPath < MAX_SMOOTH)
					{
						dtVcopy(&m_smoothPath[m_nsmoothPath * 3], iterPos);
						m_nsmoothPath++;
					}
					break;
				}
				else if (offMeshConnection && inRange(iterPos, steerPos, SLOP, 1.0f))
				{
					// Reached off-mesh connection.
					float startPos[3], endPos[3];

					// Advance the path up to and over the off-mesh connection.
					dtPolyRef prevRef = 0, polyRef = polys[0];
					int npos = 0;
					while (npos < npolys && polyRef != steerPosRef)
					{
						prevRef = polyRef;
						polyRef = polys[npos];
						npos++;
					}
					for (int i = npos; i < npolys; ++i)
						polys[i - npos] = polys[i];
					npolys -= npos;

					// Handle the connection.
					dtStatus status = NavMesh::Get()->GetNavMesh()->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos);
					if (dtStatusSucceed(status))
					{
						if (m_nsmoothPath < MAX_SMOOTH)
						{
							dtVcopy(&m_smoothPath[m_nsmoothPath * 3], startPos);
							m_nsmoothPath++;
							// Hack to make the dotted path not visible during off-mesh connection.
							if (m_nsmoothPath & 1)
							{
								dtVcopy(&m_smoothPath[m_nsmoothPath * 3], startPos);
								m_nsmoothPath++;
							}
						}
						// Move position at the other side of the off-mesh link.
						dtVcopy(iterPos, endPos);
						float eh = 0.0f;
						m_navQuery->getPolyHeight(polys[0], iterPos, &eh);
						iterPos[1] = eh;
					}
				}

				// Store results.
				if (m_nsmoothPath < MAX_SMOOTH)
				{
					dtVcopy(&m_smoothPath[m_nsmoothPath * 3], iterPos);
					m_nsmoothPath++;
				}
			}
		}

	}
	else
	{
		m_npolys = 0;
		m_nsmoothPath = 0;
	}

	for (int i = 0; i < m_nsmoothPath; i++) {
		//result.push_back(Vector3(&m_smoothPath[i * 3]));
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
