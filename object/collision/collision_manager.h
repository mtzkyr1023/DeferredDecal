#ifndef _COLLISION_MANAGER_H_
#define _COLLISION_MANAGER_H_

#include <memory>
#include <unordered_map>
#include <functional>

#include "btBulletDynamicsCommon.h"
#include "BulletDynamics/Character/btCharacterControllerInterface.h"

#include "collision_rigid_body.h"
#include "collision_ghost_object.h"
#include "collision_character_controller.h"

enum class CollisionMask {
	eGround = 1 << 0,
	ePlayer = 1 << 1,
	eEnemy = 1 << 2,
	ePlayerShot = 1 << 3,
	eEnemyShot = 1 << 4,
};


class CollisionManager {
private:
	CollisionManager();
	~CollisionManager();


public:

	void simulate(float deltaTime);

	static CollisionManager& instance() {
		static CollisionManager inst;
		return inst;
	}

	void addRigidBody(CollisionRigidBody* rigidBody);
	void addCollisionObject(CollisionGhostObject* ghostObject);
	void addCharacterController(CollisionCharacterController* characterController);

	void removeRigidBody(CollisionRigidBody* rigidBody);
	void removeCollisionObject(CollisionGhostObject* ghostObject);
	void removeCharacterController(CollisionCharacterController* characterController);


	void resetOnCollisionEnterFunction(btCollisionObject* object, const std::function<void(int)>& func);
	void resetOnCollisionStayFunction(btCollisionObject* object, const std::function<void(int)>& func);
	void resetOnCollisionExitFunction(btCollisionObject* object, const std::function<void(int)>& func);

	btDiscreteDynamicsWorld* getWorld() { return m_world.get(); }

private:
	std::unique_ptr<btDiscreteDynamicsWorld> m_world;

	std::unique_ptr<btBroadphaseInterface> m_broadphase;
	
	std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfigration;

	std::unique_ptr<btCollisionDispatcher> m_collisionDispatcher;

	std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;

	std::unique_ptr<btOverlappingPairCallback> m_pairCallback;

	std::unordered_map<btCollisionObject*, bool> m_isEnter;
	std::unordered_map<btCollisionObject*, bool> m_isExit;

	std::unordered_map<btCollisionObject*, std::function<void(int)>> m_onCollisionEnter;
	std::unordered_map<btCollisionObject*, std::function<void(int)>> m_onCollisionStay;
	std::unordered_map<btCollisionObject*, std::function<void(int)>> m_onCollisionExit;

private:
	struct SimulationContactResultCallback : public btCollisionWorld::ContactResultCallback
	{

		bool bCollision;

		SimulationContactResultCallback() : bCollision(false)
		{}


		btScalar addSingleResult(btManifoldPoint& cp,
			const btCollisionObjectWrapper* colObj0Wrap,
			int partId0,
			int index0,
			const btCollisionObjectWrapper* colObj1Wrap,
			int partId1,
			int index1)
		{
			if (cp.getDistance() <= 0.0f)
				bCollision = true;;
			
			return cp.getDistance();
		}
	};
};

#endif