#pragma once

#include <memory>
#include "SMath.h"
#include "System.h"
#include "GameEvents.h"

class Mesh;
class rcContext;
class rcHeightfield;
class rcCompactHeightfield;
class rcContourSet;
class rcPolyMesh;
class rcPolyMeshDetail;
class dtNavMesh;
class dtNavMeshQuery;
class DebugDrawer;

enum SamplePartitionType
{
	SAMPLE_PARTITION_WATERSHED,
	SAMPLE_PARTITION_MONOTONE,
	SAMPLE_PARTITION_LAYERS,
};

enum SamplePolyAreas
{
	SAMPLE_POLYAREA_GROUND,
	SAMPLE_POLYAREA_WATER,
	SAMPLE_POLYAREA_ROAD,
	SAMPLE_POLYAREA_DOOR,
	SAMPLE_POLYAREA_GRASS,
	SAMPLE_POLYAREA_JUMP,
};

enum SamplePolyFlags
{
	SAMPLE_POLYFLAGS_WALK = 0x01,		// Ability to walk (ground, grass, road)
	SAMPLE_POLYFLAGS_SWIM = 0x02,		// Ability to swim (water).
	SAMPLE_POLYFLAGS_DOOR = 0x04,		// Ability to move through doors.
	SAMPLE_POLYFLAGS_JUMP = 0x08,		// Ability to jump.
	SAMPLE_POLYFLAGS_DISABLED = 0x10,		// Disabled polygon
	SAMPLE_POLYFLAGS_ALL = 0xffff	// All abilities.
};

//TODO Navigation is System but not NavMesh
class NavMesh : public System<NavMesh>{
public:
	void Build();

	virtual bool Init() override;
	virtual void Term()override;

	virtual void Draw() override;

	dtNavMeshQuery* GetQuery() { return m_navQuery; }
	dtNavMesh* GetNavMesh() { return m_navMesh; }
private:
	void BuildForSingleMesh(const std::shared_ptr<Mesh>& mesh, const Matrix4& transform);
	bool BuildAllMeshes();

	void LoadOrBuild();

	bool RecreateEmptyNavmesh();

	void Save();
	bool Load();

	float m_cellSize = 0.3;
	float m_cellHeight = 0.1;
	float m_agentMaxSlope = 30.f;
	float m_agentHeight = 1.5f;
	float m_agentMaxClimb = 0.3f;
	float m_agentRadius = 0.3f;
	float m_edgeMaxLen = 1.f;
	float m_edgeMaxError = 0.1f;
	float m_regionMinSize = 0.1f;
	float m_regionMergeSize = 0.5f;
	int m_vertsPerPoly = 6;
	float m_detailSampleDist = 1.05f;
	float m_detailSampleMaxError = 0.1f;


	bool m_filterLowHangingObstacles = false;
	bool m_filterLedgeSpans = false;
	bool m_filterWalkableLowHeightSpans = false;

	int m_partitionType = SAMPLE_PARTITION_WATERSHED;

	std::vector<Vector3> vertices;
	std::vector<int> tris;
	AABB aabb;

	unsigned char* m_triareas;
	rcHeightfield* m_solid;
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;
	rcContext* m_ctx;
	rcPolyMeshDetail* m_dmesh;

	dtNavMesh* m_navMesh;
	dtNavMeshQuery* m_navQuery;

	DebugDrawer* debugDrawer;

	GameEventHandle onSceneLoadedHandler;
};