

#include "GhostBody.h"
#include "GameObject.h"
#include "Transform.h"
#include "Collider.h"
#include "PhysicsSystem.h"
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "Common.h"
#include "Camera.h"
#include "MeshRenderer.h"
#include "Physics.h"
#include <BulletCollision\CollisionDispatch\btGhostObject.h>

DECLARE_TEXT_ASSET(GhostBody);
//TODO add trigger enter/exit events

// Portable static method: prerequisite call: m_dynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback()); 
void GetCollidingObjectsInsidePairCachingGhostObject(btDynamicsWorld* m_dynamicsWorld, btPairCachingGhostObject* m_pairCachingGhostObject, btAlignedObjectArray < btCollisionObject* >& collisionArrayOut) {
	collisionArrayOut.resize(0);
	if (!m_pairCachingGhostObject || !m_dynamicsWorld) return;
	const bool addOnlyObjectsWithNegativeDistance(true);	// With "false" things don't change much, and the code is a bit faster and cleaner...

	//#define USE_PLAIN_COLLISION_WORLD // We dispatch all collision pairs of the ghost object every step (slow)
#ifdef USE_PLAIN_COLLISION_WORLD
//======================================================================================================
// I thought this line was no longer needed, but it seems to be necessary (and I believe it's an expensive call):
	m_dynamicsWorld->getDispatcher()->dispatchAllCollisionPairs(m_pairCachingGhostObject->getOverlappingPairCache(), m_dynamicsWorld->getDispatchInfo(), m_dynamicsWorld->getDispatcher());
	// Maybe the call can be automatically triggered by some other Bullet call (I'm almost sure I could comment it out in another demo I made long ago...)
	// So by now the general rule is: in real projects, simply comment it out and see if it works!
	//======================================================================================================
	// UPDATE: in dynamic worlds, the line above can be commented out and the broadphase pair can be retrieved through the call to findPair(...) below.
	// In collision worlds probably the above line is needed only if collision detection for all the bodies hasn't been made... This is something
	// I'm still not sure of... the general rule is to try to comment out the line above and try to use findPair(...) and see if it works whenever possible....
	//======================================================================================================
#endif //USE_PLAIN_COLLISION_WORLD

	btBroadphasePairArray& collisionPairs = m_pairCachingGhostObject->getOverlappingPairCache()->getOverlappingPairArray();
	const int	numObjects = collisionPairs.size();
	static btManifoldArray	m_manifoldArray;
	bool added;
	for (int i = 0; i < numObjects; i++) {
		m_manifoldArray.resize(0);

#ifdef USE_PLAIN_COLLISION_WORLD
		const btBroadphasePair& collisionPair = collisionPairs[i];
		if (collisionPair.m_algorithm) collisionPair.m_algorithm->getAllContactManifolds(m_manifoldArray);
		else {	// THIS SHOULD NEVER HAPPEN, AND IF IT DOES, PLEASE RE-ENABLE the "call" a few lines above...
			printf("No collisionPair.m_algorithm - probably m_dynamicsWorld->getDispatcher()->dispatchAllCollisionPairs(...) must be missing.\n");
		}
#else // USE_PLAIN_COLLISION_WORLD	
		const btBroadphasePair& cPair = collisionPairs[i];
		//unless we manually perform collision detection on this pair, the contacts are in the dynamics world paircache:
		const btBroadphasePair* collisionPair = m_dynamicsWorld->getPairCache()->findPair(cPair.m_pProxy0, cPair.m_pProxy1);
		if (!collisionPair) continue;
		if (collisionPair->m_algorithm) collisionPair->m_algorithm->getAllContactManifolds(m_manifoldArray);
		else {	// THIS SHOULD NEVER HAPPEN, AND IF IT DOES, PLEASE RE-ENABLE the "call" a few lines above...
			printf("No collisionPair.m_algorithm - probably m_dynamicsWorld->getDispatcher()->dispatchAllCollisionPairs(...) must be missing.\n");
		}
#endif //USE_PLAIN_COLLISION_WORLD

		added = false;
		for (int j = 0; j < m_manifoldArray.size(); j++) {
			btPersistentManifold* manifold = m_manifoldArray[j];
			// Here we are in the narrowphase, but can happen that manifold->getNumContacts()==0:
			if (addOnlyObjectsWithNegativeDistance) {
				for (int p = 0, numContacts = manifold->getNumContacts(); p < numContacts; p++) {
					const btManifoldPoint& pt = manifold->getContactPoint(p);
					if (pt.getDistance() < 0.0) {
						// How can I be sure that the colObjs are all distinct ? I use the "added" flag.
						collisionArrayOut.push_back((btCollisionObject*)(manifold->getBody0() == m_pairCachingGhostObject ? manifold->getBody1() : manifold->getBody0()));
						added = true;
						break;
					}
				}
				if (added) break;
			}
			else if (manifold->getNumContacts() > 0) {
				collisionArrayOut.push_back((btCollisionObject*)(manifold->getBody0() == m_pairCachingGhostObject ? manifold->getBody1() : manifold->getBody0()));
				break;
			}
		}
	}
}

void GhostBody::OnEnable() {
	auto collider = gameObject()->GetComponent<Collider>();
	if (!collider) {
		return;
	}

	auto shape = collider->shape;
	if (!shape) {
		return;
	}

	transform = gameObject()->transform().get();
	if (!transform) {
		return;
	}
	auto matr = transform->matrix;
	SetScale(matr, Vector3_one);
	btTransform groundTransform = btConvert(matr);

	//TODO why pair caching ?
	pBody = new btPairCachingGhostObject();
	pBody->setCollisionShape(shape.get());
	pBody->setWorldTransform(groundTransform);
	pBody->setUserPointer(this);

	//add the body to the dynamics world
	int group;
	int mask;
	pBody->setCollisionFlags(Bits::SetMaskTrue(pBody->getCollisionFlags(), btCollisionObject::CF_NO_CONTACT_RESPONSE));
	pBody->setCollisionFlags(Bits::SetMaskTrue(pBody->getCollisionFlags(), btCollisionObject::CF_KINEMATIC_OBJECT));//TODO static for static
	PhysicsSystem::Get()->GetGroupAndMask(layer, group, mask);
	PhysicsSystem::Get()->dynamicsWorld->addCollisionObject(pBody, group, mask);
}

void GhostBody::FixedUpdate() {
	//TODO remove fixed update for static objects
	//TODO disable fixedUpdate flag if no body
	if (!pBody) {
		return;
	}
	auto matr = transform->matrix;
	SetScale(matr, Vector3_one);
	btTransform groundTransform = btConvert(matr);
	if (!(pBody->getWorldTransform() == groundTransform)) {
		pBody->setWorldTransform(groundTransform);
	}
}

void GhostBody::OnDisable() {
	if (pBody) {
		PhysicsSystem::Get()->dynamicsWorld->removeCollisionObject(pBody);
	}
	SAFE_DELETE(pBody);
}

int GhostBody::GetOverlappedCount() const {
	if (!pBody) {
		return 0;
	}
	btAlignedObjectArray < btCollisionObject* > collisionArrayOut;
	GetCollidingObjectsInsidePairCachingGhostObject(PhysicsSystem::Get()->dynamicsWorld, pBody, collisionArrayOut);

	return collisionArrayOut.size();
}

std::shared_ptr<GameObject> GhostBody::GetOverlappedObject(int idx) const {
	btAlignedObjectArray < btCollisionObject* > collisionArrayOut;
	GetCollidingObjectsInsidePairCachingGhostObject(PhysicsSystem::Get()->dynamicsWorld, pBody, collisionArrayOut);
	//TODO optimize
	return Physics::GetGameObject(collisionArrayOut[idx]);
}
