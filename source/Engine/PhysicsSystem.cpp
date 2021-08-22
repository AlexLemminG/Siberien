#include "PhysicsSystem.h"
#include "btBulletDynamicsCommon.h"
#include <stdio.h>
#include "STime.h"
#include "Scene.h"
#include "Common.h"
#include "GameObject.h"
#include "Mesh.h"
#include "MeshCollider.h"//TODO move meshPhysicsData to separate class
#include <BulletCollision\CollisionDispatch\btGhostObject.h>
#include "Bullet3Serialize/Bullet2FileLoader/b3BulletFile.h"
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.h"

REGISTER_SYSTEM(PhysicsSystem);

bool PhysicsSystem::Init() {

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

	dynamicsWorld = new btDiscreteDynamicsWorldMt(dispatcher, overlappingPairCache, solverPool, solver, collisionConfiguration);

	dynamicsWorld->setGravity(btConvert(GetGravity()));

	dynamicsWorld->setInternalTickCallback(OnPhysicsTick);

	///-----initialization_end-----


	/// Do some simulation

	///-----stepsimulation_start-----
	prevSimulationTime = Time::time();

	return true;
}

void PhysicsSystem::Update() {
	OPTICK_EVENT();
	dynamicsWorld->stepSimulation(Time::deltaTime(), 2, Time::fixedDeltaTime());
}

void PhysicsSystem::Term() {

	//delete dynamics world
	delete dynamicsWorld;

	//delete solver
	delete solver;

	delete solverPool;

	//delete broadphase
	delete overlappingPairCache;

	//delete dispatcher
	delete dispatcher;

	delete collisionConfiguration;

	btSetTaskScheduler(nullptr);
	delete taskScheduler;
}

Vector3 PhysicsSystem::GetGravity() const {
	return Vector3(0, -10, 0);
}

void PhysicsSystem::GetGroupAndMask(const std::string& groupName, int& group, int& mask) {
	if (groupName.empty() || groupName == "default") {
		group = defaultGroup;
		mask = defaultMask;
	}
	else if (groupName == "player") {
		group = playerGroup;
		mask = playerMask;
	}
	else  if (groupName == "playerBullet") {
		group = playerBulletGroup;
		mask = playerBulletMask;
	}
	else  if (groupName == "enemy") {
		group = enemyGroup;
		mask = enemyMask;
	}
	else  if (groupName == "enemyBullet") {
		group = enemyBulletGroup;
		mask = enemyBulletMask;
	}
	else  if (groupName == "enemyCorpse") {
		group = enemyCorpseGroup;
		mask = enemyCorpseMask;
	}
	else  if (groupName == "grenade") {
		group = grenadeGroup;
		mask = grenadeMask;
	}
	else  if (groupName == "staticGeom") {
		group = staticGeomGroup;
		mask = staticGeomMask;
	}
	else {
		LogError("Unknown group name %s", groupName.c_str());
		group = defaultGroup;
		mask = defaultMask;
	}

}

void PhysicsSystem::SerializeMeshPhysicsDataToBuffer(std::vector<std::shared_ptr<Mesh>>& meshes, BinaryBuffer& buffer) {
	std::vector<uint8_t> tempBuffer; //TODO need for aligning only (and not realy correct)
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
		tempBuffer.resize(size);
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
			mesh->physicsData->triangleShapeBuffer.resize(size);//TODO dich
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


class GetAllContacts_ContactResultCallback : public btCollisionWorld::ContactResultCallback {
public:
	// Inherited via ContactResultCallback
	virtual btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override
	{
		objects.push_back(const_cast<btCollisionObject*>(colObj0Wrap->getCollisionObject()));
		return btScalar();
	}
	std::vector<btCollisionObject*> objects;
};

std::vector<GameObject*> PhysicsSystem::GetOverlaping(Vector3 pos, float radius) {
	std::vector<GameObject*> results;

	btSphereShape sphere(radius);
	btPairCachingGhostObject ghost;
	btTransform xform;
	xform.setOrigin(btConvert(pos));
	ghost.setCollisionShape(&sphere);
	ghost.setWorldTransform(xform);

	GetAllContacts_ContactResultCallback cb;
	cb.m_collisionFilterGroup = PhysicsSystem::playerBulletGroup;
	cb.m_collisionFilterMask = PhysicsSystem::playerBulletMask;
	PhysicsSystem::Get()->dynamicsWorld->contactTest(&ghost, cb);

	//TODO may contain doubles 

	for (auto o : cb.objects) {
		auto* rb = dynamic_cast<btRigidBody*>(o);
		if (rb) {
			auto ptr = rb->getUserPointer();
			auto go = (GameObject*)(ptr);
			if (go) {
				results.push_back(go);
			}
		}
	}
	PhysicsSystem::Get()->dynamicsWorld->removeCollisionObject(&ghost);

	return results;
}