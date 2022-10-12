#include "collision_manager.h"

#include "BulletCollision/CollisionDispatch/btGhostObject.h"


CollisionManager::CollisionManager() {
	m_broadphase = std::make_unique<btDbvtBroadphase>();

	m_pairCallback = std::make_unique<btGhostPairCallback>();

	m_broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(m_pairCallback.get());

	m_collisionConfigration = std::make_unique<btDefaultCollisionConfiguration>();
	m_collisionDispatcher = std::make_unique<btCollisionDispatcher>(m_collisionConfigration.get());

	m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();

	m_world = std::make_unique<btDiscreteDynamicsWorld>(m_collisionDispatcher.get(), m_broadphase.get(), m_solver.get(), m_collisionConfigration.get());
}

CollisionManager::~CollisionManager() {
	m_broadphase.reset();
	m_collisionConfigration.reset();
	m_collisionDispatcher.reset();
	m_solver.reset();
	m_world.reset(); 
}


void CollisionManager::simulate(float deltaTime) {

	m_world->stepSimulation(deltaTime, 16);

	btBroadphasePairArray& pairArray = m_broadphase->getOverlappingPairCache()->getOverlappingPairArray();

	for (int i = 0; i < m_broadphase->getOverlappingPairCache()->getNumOverlappingPairs(); i++) {

		btBroadphasePair& pair = pairArray[i];

		bool collides = (pair.m_pProxy0->m_collisionFilterGroup & pair.m_pProxy1->m_collisionFilterMask) != 0;
		collides = collides && (pair.m_pProxy1->m_collisionFilterGroup & pair.m_pProxy0->m_collisionFilterMask);

		btCollisionObject* object0 = (btCollisionObject*)pair.m_pProxy0->m_clientObject;
		btCollisionObject* object1 = (btCollisionObject*)pair.m_pProxy1->m_clientObject;



		SimulationContactResultCallback result;

		m_world->contactPairTest(object0, object1, result);

		if (collides && result.bCollision && !m_isEnter[object0] && m_onCollisionEnter[object0]) {
			m_onCollisionEnter[object0](pairArray[i].m_pProxy1->m_collisionFilterMask);
			m_isEnter[object0] = true;
		}
		else if (collides && result.bCollision && m_isEnter[object0] && m_onCollisionEnter[object0]) {
			m_onCollisionEnter[object0](pairArray[i].m_pProxy1->m_collisionFilterMask);
		}
		else if (collides && !result.bCollision && m_isEnter[object0] && m_onCollisionExit[object0]) {
			m_onCollisionExit[object0](pairArray[i].m_pProxy1->m_collisionFilterMask);
			m_isEnter[object0] = false;
		}

		if (collides && result.bCollision && !m_isEnter[object1] && m_onCollisionEnter[object1]) {
			m_onCollisionEnter[object1](pairArray[i].m_pProxy0->m_collisionFilterMask);
			m_isEnter[object1] = true;
		}
		else if (collides && result.bCollision && m_isEnter[object1] && m_onCollisionEnter[object1]) {
			m_onCollisionEnter[object1](pairArray[i].m_pProxy0->m_collisionFilterMask);
		}
		else if (collides && !result.bCollision && m_isEnter[object1] && m_onCollisionExit[object1]) {
			m_onCollisionExit[object1](pairArray[i].m_pProxy0->m_collisionFilterMask);
			m_isEnter[object1] = false;
		}
	}
}

void CollisionManager::addRigidBody(CollisionRigidBody* rigidBody) {
	m_world->addRigidBody(rigidBody->rigidBody(), rigidBody->group(), rigidBody->mask());
	m_onCollisionEnter[rigidBody->rigidBody()] = rigidBody->onCollisionEnter();
	m_onCollisionStay[rigidBody->rigidBody()] = rigidBody->onCollisionEnter();
	m_onCollisionExit[rigidBody->rigidBody()] = rigidBody->onCollisionExit();
	m_isExit[rigidBody->rigidBody()] = true;
	m_isEnter[rigidBody->rigidBody()] = false;
}

void CollisionManager::addCollisionObject(CollisionGhostObject* ghostObject) {
	m_world->addCollisionObject(ghostObject->ghostObject(), ghostObject->group(), ghostObject->mask());
	m_onCollisionEnter[ghostObject->ghostObject()] = ghostObject->onCollisionEnter();
	m_onCollisionStay[ghostObject->ghostObject()] = ghostObject->onCollisionEnter();
	m_onCollisionExit[ghostObject->ghostObject()] = ghostObject->onCollisionExit();
	m_isExit[ghostObject->ghostObject()] = true;
	m_isEnter[ghostObject->ghostObject()] = false;
}

void CollisionManager::addCharacterController(CollisionCharacterController* characterController) {
	m_world->addCollisionObject(characterController->ghostObject(), characterController->group(), characterController->mask());
	m_onCollisionEnter[characterController->ghostObject()] = characterController->onCollisionEnter();
	m_onCollisionStay[characterController->ghostObject()] = characterController->onCollisionEnter();
	m_onCollisionExit[characterController->ghostObject()] = characterController->onCollisionExit();
	m_isExit[characterController->ghostObject()] = true;
	m_isEnter[characterController->ghostObject()] = false;
	m_world->addCharacter(characterController->characterController());
}

void CollisionManager::removeCharacterController(CollisionCharacterController* characterController) {
	m_world->removeCollisionObject(characterController->ghostObject());
	m_world->removeCharacter(characterController->characterController());
}

void CollisionManager::removeRigidBody(CollisionRigidBody* rigidBody) {
	m_world->removeRigidBody(rigidBody->rigidBody());
}

void CollisionManager::removeCollisionObject(CollisionGhostObject* ghostObject) {
	m_world->removeCollisionObject(ghostObject->ghostObject());
}


void CollisionManager::resetOnCollisionEnterFunction(btCollisionObject* object, const std::function<void(int)>& func) {
	m_onCollisionEnter[object] = func;
}

void CollisionManager::resetOnCollisionStayFunction(btCollisionObject* object, const std::function<void(int)>& func) {
	m_onCollisionStay[object] = func;
}

void CollisionManager::resetOnCollisionExitFunction(btCollisionObject* object, const std::function<void(int)>& func) {
	m_onCollisionExit[object] = func;
}