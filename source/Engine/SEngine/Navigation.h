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
class dtCrowd;

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
	virtual void Update() override;
	virtual void Term()override;

	virtual void Draw() override;

	dtNavMeshQuery* GetQuery() { return m_navQuery; }
	dtNavMesh* GetNavMesh() { return m_navMesh; }
	dtCrowd* GetCrowd() { return m_crowd; }
private:
	void BuildForSingleMesh(const std::shared_ptr<Mesh>& mesh, const Matrix4& transform);
	bool BuildAllMeshes();

	void LoadOrBuild();

	bool RecreateEmptyNavmesh();
	void RecreateCrowd();

	void Save();
	bool Load();

	float m_cellSize = 0.15;
	float m_cellHeight = 0.15;
	float m_agentMaxSlope = 30.f;
	float m_agentHeight = 2.f;
	float m_agentMaxClimb = 0.9f;
	float m_agentRadius = 0.666f;
	float m_edgeMaxLen = 30.f;
	float m_edgeMaxError = 1.2f;
	float m_regionMinSize = 8.0f;
	float m_regionMergeSize = 20.f;
	int m_vertsPerPoly = 6;
	float m_detailSampleDist = 6.f;
	float m_detailSampleMaxError = 1.f;


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
	dtCrowd* m_crowd = nullptr;

	dtNavMesh* m_navMesh;
	dtNavMeshQuery* m_navQuery;

	DebugDrawer* debugDrawer;

	GameEventHandle onSceneLoadedHandler;
};