#include "collision_character_controller.h"

#include "collision_manager.h"

#include "BulletDynamics/Character/btKinematicCharacterController.h"

CollisionCharacterController::CollisionCharacterController() {

}

CollisionCharacterController::CollisionCharacterController(float radius, float height, int group, int mask, const glm::vec3& position, const glm::vec3& rotation, float step) :
	m_group(group),
	m_mask(mask)
{
	m_colShape = std::make_unique<btCapsuleShape>(radius, height);

	m_ghostObject = std::make_unique<btPairCachingGhostObject>();

	btTransform startTransform;
	startTransform.setIdentity();
	startTransform.setOrigin(btVector3(position.x, position.y, position.z));
	startTransform.setRotation(btQuaternion(glm::radians(rotation.x), glm::radians(rotation.y), glm::radians(rotation.z)));

	m_ghostObject->setWorldTransform(startTransform);
	m_ghostObject->setCollisionShape(m_colShape.get());

	m_ghostObject->setCollisionFlags(
		btCollisionObject::CF_KINEMATIC_OBJECT |
		btCollisionObject::CF_STATIC_OBJECT |
		btCollisionObject::CF_DYNAMIC_OBJECT
	);

	m_characterController = std::make_unique<btKinematicCharacterController>(static_cast<btPairCachingGhostObject*>(m_ghostObject.get()),
		static_cast<btCapsuleShape*>(m_colShape.get()), btScalar(step), btVector3(0.0f, 1.0f, 0.0f));

	m_characterController->setGravity(CollisionManager::instance().getWorld()->getGravity());

	CollisionManager::instance().addCharacterController(this);
}


CollisionCharacterController::~CollisionCharacterController() {
	if (m_characterController)
		CollisionManager::instance().removeCharacterController(this);
}


void CollisionCharacterController::setOnEnterFunction(const std::function<void(int)>& func) {
	m_onCollisionEnter = func;
	CollisionManager::instance().resetOnCollisionEnterFunction(m_ghostObject.get(), func);
}

void CollisionCharacterController::setOnStayFunction(const std::function<void(int)>& func) {
	m_onCollisionStay = func;
	CollisionManager::instance().resetOnCollisionStayFunction(m_ghostObject.get(), func);
}

void CollisionCharacterController::setOnExitFunction(const std::function<void(int)>& func) {
	m_onCollisionExit = func;
	CollisionManager::instance().resetOnCollisionExitFunction(m_ghostObject.get(), func);
}