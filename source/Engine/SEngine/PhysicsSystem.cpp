

#include "PhysicsSystem.h"
#include "btBulletDynamicsCommon.h"
#include <stdio.h>
#include "STime.h"
#include "Scene.h"
#include "Common.h"
#include "GameObject.h"
#include "Mesh.h"
#include "MeshCollider.h"//TODO move meshPhysicsData to separate class
#include "Resources.h"
#include "Bullet3Serialize/Bullet2FileLoader/b3BulletFile.h"
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.h"
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include "Dbg.h"
#include "DbgVars.h"
#include "Camera.h"
#include "AlignedAllocator.h"


DBG_VAR_BOOL(dbg_drawPhysShapes, "draw phys shapes", false);

REGISTER_SYSTEM(PhysicsSystem);
DECLARE_TEXT_ASSET(PhysicsSettings);

class DebugDraw : public btIDebugDraw {
	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
		Color c;
		c.r = color.x();
		c.g = color.y();
		c.b = color.z();
		c.a = 1.f;
		Dbg::DrawLine(btConvert(from), btConvert(to), c);
	}

	virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {
		Dbg::Draw(btConvert(PointOnB), distance);
	}

	virtual void reportErrorWarning(const char* warningString) {}

	virtual void draw3dText(const btVector3& location, const char* textString) {}

	virtual void setDebugMode(int debugMode) { this->debugMode = debugMode; }

	virtual int getDebugMode() const {
		return debugMode;
	}
	int debugMode = 0;
};

bool PhysicsSystem::Init() {
	settings = AssetDatabase::Get()->Load<PhysicsSettings>("settings.asset");
	if (settings == nullptr) {
		settings = std::make_shared<PhysicsSettings>();//TODO probably need better default settings
	}
	///-----includes_end-----

	int i;
	///-----initialization_start-----

	taskScheduler = btCreateDefaultTaskScheduler();
	//taskScheduler->setNumThreads(taskScheduler->getMaxNumThreads());
	btSetTaskScheduler(taskScheduler);

	btDefaultCollisionConstructionInfo cci;
	///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
	collisionConfiguration = new btDefaultCollisionConfiguration(cci);

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	dispatcher = new btCollisionDispatcherMt(collisionConfiguration);

	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	overlappingPairCache = new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	solver = new btSequentialImpulseConstraintSolverMt();

	solverPool = new btConstraintSolverPoolMt(4);

	ghostCall = new btGhostPairCallback();
	overlappingPairCache->getOverlappingPairCache()->setInternalGhostPairCallback(ghostCall);


	dynamicsWorld = new btDiscreteDynamicsWorldMt(dispatcher, overlappingPairCache, solverPool, solver, collisionConfiguration);

	dynamicsWorld->setGravity(btConvert(settings->gravity));

	dynamicsWorld->setInternalTickCallback(OnPhysicsTick);

	///-----initialization_end-----


	/// Do some simulation

	///-----stepsimulation_start-----
	prevSimulationTime = Time::time();

	//TODO delete
	dynamicsWorld->setDebugDrawer(new DebugDraw());

	return true;
}

void PhysicsSystem::Update() {
	OPTICK_EVENT();
	if (Scene::Get()) {
		if (Scene::Get()->IsInEditMode()) {
			auto wasSyncAll = dynamicsWorld->getSynchronizeAllMotionStates();
			dynamicsWorld->setSynchronizeAllMotionStates(true);
			dynamicsWorld->synchronizeMotionStates();
			dynamicsWorld->setSynchronizeAllMotionStates(wasSyncAll);
		}
		else {
			dynamicsWorld->stepSimulation(Time::deltaTime(), 2, Time::fixedDeltaTime());
		}
	}

#ifdef SE_HAS_DEBUG
	auto dbgMode = Bits::SetMask(dynamicsWorld->getDebugDrawer()->getDebugMode(), btIDebugDraw::DBG_DrawWireframe, dbg_drawPhysShapes);
	dynamicsWorld->getDebugDrawer()->setDebugMode(dbgMode);
	if (dbg_drawPhysShapes) {
		//manual culling
		auto collisionObjects = dynamicsWorld->getCollisionObjectArray();
		for (int i = 0; i < collisionObjects.size(); i++) {
			auto& pBody = collisionObjects.at(i);
			auto camera = Camera::GetMain();
			if (camera) {
				btVector3 min;
				btVector3 max;
				pBody->getCollisionShape()->getAabb(pBody->getWorldTransform(), min, max);
				AABB aabb{ btConvert(min),btConvert(max) };
				if (camera->IsVisible(aabb)) {
					pBody->setCollisionFlags(Bits::SetMaskFalse(pBody->getCollisionFlags(), btCollisionObject::CollisionFlags::CF_DISABLE_VISUALIZE_OBJECT));
				}
				else {
					pBody->setCollisionFlags(Bits::SetMaskTrue(pBody->getCollisionFlags(), btCollisionObject::CollisionFlags::CF_DISABLE_VISUALIZE_OBJECT));
				}
			}
			else {
				pBody->setCollisionFlags(Bits::SetMaskTrue(pBody->getCollisionFlags(), btCollisionObject::CollisionFlags::CF_DISABLE_VISUALIZE_OBJECT));
			}
		}
		dynamicsWorld->debugDrawWorld();
	}
#endif
}

void PhysicsSystem::Term() {

	SAFE_DELETE(dynamicsWorld);

	SAFE_DELETE(solver);

	SAFE_DELETE(solverPool);

	//delete broadphase
	SAFE_DELETE(overlappingPairCache);

	//delete dispatcher
	SAFE_DELETE(dispatcher);

	SAFE_DELETE(collisionConfiguration);

	SAFE_DELETE(ghostCall);

	btSetTaskScheduler(nullptr);
	SAFE_DELETE(taskScheduler);
}

Vector3 PhysicsSystem::GetGravity() const {
	return btConvert(dynamicsWorld->getGravity());
}

void PhysicsSystem::GetGroupAndMask(const std::string& groupName, int& group, int& mask) {
	return settings->GetGroupAndMask(groupName, group, mask);
}

void PhysicsSystem::SerializeMeshPhysicsDataToBuffer(std::vector<std::shared_ptr<Mesh>>& meshes, BinaryBuffer& buffer) {
	std::vector<uint8_t, aligned_allocator<uint8_t, 16>> tempBuffer; //need for aligning only
	for (const auto& mesh : meshes) {
		bool hasData = mesh->physicsData != nullptr;
		buffer.Write(hasData);
		if (!hasData) {
			continue;
		}
		bool hasShape = mesh->physicsData->triangleShape != nullptr;
		buffer.Write(hasShape);
		if (!hasShape) {
			continue;
		}
		auto bvh = mesh->physicsData->triangleShape->getOptimizedBvh();
		int size = bvh->calculateSerializeBufferSize();
		ResizeVectorNoInit(tempBuffer, size);
		buffer.Write(size);
		if (size) {
			bvh->serialize(&tempBuffer[0], tempBuffer.size(), false);
		}
		buffer.Write(&tempBuffer[0], tempBuffer.size());
	}
}

void PhysicsSystem::DeserializeMeshPhysicsDataFromBuffer(std::vector<std::shared_ptr<Mesh>>& meshes, BinaryBuffer& buffer) {
	for (int i = 0; i < meshes.size(); i++) {
		auto& mesh = meshes[i];
		bool hasData;
		buffer.Read(hasData);
		if (!hasData) {
			continue;
		}
		mesh->physicsData = std::make_unique<MeshPhysicsData>();
		bool hasShape;
		buffer.Read(hasShape);
		if (!hasShape) {
			continue;
		}
		int size;
		buffer.Read(size);
		if (size) {
			ResizeVectorNoInit(mesh->physicsData->triangleShapeBuffer, size);//TODO dich
			buffer.Read(&mesh->physicsData->triangleShapeBuffer[0], size);

			btOptimizedBvh* bvh = btOptimizedBvh::deSerializeInPlace(&mesh->physicsData->triangleShapeBuffer[0], size, false);

			auto nonBuildShape = MeshColliderStorageSystem::Get()->Create(mesh, false);
			mesh->physicsData->triangleShape = nonBuildShape.shape;
			mesh->physicsData->triangles = nonBuildShape.triangles;
			mesh->physicsData->triangleShape->setOptimizedBvh(bvh);
		}
	}
}

void PhysicsSystem::CalcMeshPhysicsDataFromBuffer(std::shared_ptr<Mesh> mesh) {
	auto meshShape = MeshColliderStorageSystem::Get()->Create(mesh, true);//TODO renames
	mesh->physicsData = std::make_unique<MeshPhysicsData>();
	mesh->physicsData->triangleShape = meshShape.shape;
	mesh->physicsData->triangles = meshShape.triangles;
}

void PhysicsSystem::OnPhysicsTick(btDynamicsWorld* world, btScalar timeStep) {
	//TODO update time
	SystemsManager::Get()->FixedUpdate();
	if (Scene::Get()) {
		Scene::Get()->FixedUpdate();
	}
}


void PhysicsSettings::GetGroupAndMask(const std::string& groupName, int& group, int& mask) {
	auto it = layersMap.find(groupName);
	if (it == layersMap.end()) {
		ASSERT(false);
		group = 0;
		mask = 0;
	}
	{
		group = it->second.group;
		mask = it->second.mask;
	}
}
std::string PhysicsSystem::reservedLayerName = "engineReservedLayerName";

void PhysicsSettings::OnAfterDeserializeCallback(const SerializationContext& context) {
	int firstLayerGroup = 1; //reserve 1 for collide with everything group (usefull for queries)

	auto reservedLayer = PhysicsSettings::Layer();
	reservedLayer.name = PhysicsSystem::reservedLayerName;
	reservedLayer.collideWith.push_back(PhysicsSystem::reservedLayerName);
	layers.push_back(reservedLayer);

	for (int i = 0; i < layers.size(); i++) {
		layers[i].group = 1 << (i + firstLayerGroup);
		layersMap[layers[i].name] = layers[i];
	}
	static const std::string allKeyword = "all";
	for (auto& kv : layersMap) {
		auto& layer = kv.second;
		if (layer.doNotCollideWith.size() > 0) {
			ASSERT(layer.collideWith.size() == 0);
			layer.mask = -1;
			for (const auto layerName : layer.doNotCollideWith) {
				if (layerName == allKeyword) {
					layer.mask = 0;
					break;
				}
				else {
					auto it = layersMap.find(layerName);
					if (it == layersMap.end()) {
						ASSERT(false);
						continue;
					}
					layer.mask &= (-1 ^ it->second.group);
				}
			}
		}
		else {
			layer.mask = 0;
			for (const auto layerName : layer.collideWith) {
				if (layerName == allKeyword) {
					layer.mask = -1;
					break;
				}
				else {
					auto it = layersMap.find(layerName);
					if (it == layersMap.end()) {
						ASSERT(false);
						continue;
					}
					layer.mask |= it->second.group;
				}
			}
		}
		layer.mask |= 1; // always collide with reserved fake group
	}
	if (layers.size() == 0) {
		layersMap[""] = Layer{};
	}
	else {
		layersMap[""] = layersMap[layers[0].name];
	}
}
