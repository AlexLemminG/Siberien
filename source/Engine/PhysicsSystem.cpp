#include "PhysicsSystem.h"
#include "btBulletDynamicsCommon.h"
#include <stdio.h>
#include "Time.h"
#include "Scene.h"

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

	dynamicsWorld->setGravity(btVector3(0, -10, 0));

	///-----initialization_end-----

	
	/// Do some simulation

	///-----stepsimulation_start-----
	prevSimulationTime = Time::time();


	return true;
}

void PhysicsSystem::Update() {
	while (prevSimulationTime < Time::time()) {
		float currentSimulationTime = prevSimulationTime + Time::fixedDeltaTime();

		dynamicsWorld->stepSimulation(Time::fixedDeltaTime(), 0, Time::fixedDeltaTime());

		prevSimulationTime = currentSimulationTime;

		//TODO update time
		SystemsManager::Get()->FixedUpdate();
		Scene::Get()->FixedUpdate();
	}
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
