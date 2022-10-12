#include "collision_ghost_object.h"


#include "collision_manager.h"
#include "BulletDynamics/Character/btCharacterControllerInterface.h"


CollisionGhostObject::CollisionGhostObject() {

}


CollisionGhostObject::CollisionGhostObject(float radius, float height, int group, int mask, const glm::vec3& position, const glm::vec3& rotation) :
	m_group(group),
	m_mask(mask)
{
	m_colShape = std::make_unique<btCapsuleShape>(radius, height);

	m_ghostObject = std::make_unique<btPairCachingGhostObject>();

	btTransform startTransform;
	startTransform.setIdentity();
	startTransform.setOrigin(btVector3(position.x, position.y, position.z));
	startTransform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z));

	m_ghostObject->setWorldTransform(startTransform);
	m_ghostObject->setCollisionShape(m_colShape.get());

	CollisionManager::instance().addCollisionObject(this);
}

CollisionGhostObject::~CollisionGhostObject() {
	if (m_ghostObject)
		CollisionManager::instance().removeCollisionObject(this);
}

void CollisionGhostObject::setOnEnterFunction(const std::function<void(int)>& func) {
	m_onCollisionEnter = func;
	CollisionManager::instance().resetOnCollisionEnterFunction(m_ghostObject.get(), func);
}

void CollisionGhostObject::setOnStayFunction(const std::function<void(int)>& func) {
	m_onCollisionStay = func;
	CollisionManager::instance().resetOnCollisionStayFunction(m_ghostObject.get(), func);
}

void CollisionGhostObject::setOnExitFunction(const std::function<void(int)>& func) {
	m_onCollisionExit = func;
	CollisionManager::instance().resetOnCollisionExitFunction(m_ghostObject.get(), func);
}