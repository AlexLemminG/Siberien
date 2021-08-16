#include "PhysicsSystem.h"
#include "btBulletDynamicsCommon.h"
#include <stdio.h>
#include "Time.h"
#include "Scene.h"
#include "GameObject.h"
#include <BulletCollision\CollisionDispatch\btGhostObject.h>

REGISTER_SYSTEM(PhysicsSystem);

bool PhysicsSystem::Init() {

	///-----includes_end-----

	int i;
	///-----initialization_start-----

	///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
	collisionConfiguration = new btDefaultCollisionConfiguration();

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	dispatcher = new btCollisionDispatcher(collisionConfiguration);

	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	overlappingPairCache = new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	solver = new btSequentialImpulseConstraintSolver();


	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);

	dynamicsWorld->setGravity(btConvert(GetGravity()));

	dynamicsWorld->setInternalTickCallback(OnPhysicsTick);

	///-----initialization_end-----


	/// Do some simulation

	///-----stepsimulation_start-----
	prevSimulationTime = Time::time();





	return true;
}

void PhysicsSystem::Update() {
	dynamicsWorld->stepSimulation(Time::deltaTime(), 2, Time::fixedDeltaTime());
}

void PhysicsSystem::Term() {

	//delete dynamics world
	delete dynamicsWorld;

	//delete solver
	delete solver;

	//delete broadphase
	delete overlappingPairCache;

	//delete dispatcher
	delete dispatcher;

	delete collisionConfiguration;
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