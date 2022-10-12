#include "collision_rigid_body.h"
#include "collision_manager.h"

#include "btBulletDynamicsCommon.h"

CollisionRigidBody::CollisionRigidBody() {
}

CollisionRigidBody::CollisionRigidBody(float radius, float mass, int group, int mask, const glm::vec3& initialPosition,
	const glm::vec3& initialRotation) :
	m_group(group),
	m_mask(mask)
{
	m_colShape = std::make_unique<btSphereShape>(btScalar(radius));

	btScalar    m(mass);

	bool isDynamic = (mass != 0.f);

	btVector3 localInertia(0, 0, 0);
	if (isDynamic)
		m_colShape->calculateLocalInertia(m, localInertia);

	btTransform startTransform;
	startTransform.setIdentity();
	startTransform.setOrigin(btVector3(initialPosition.x, initialPosition.y, initialPosition.z));
	startTransform.setRotation(btQuaternion(initialRotation.x, initialRotation.y, initialRotation.z));

	m_motionState = std::make_unique<btDefaultMotionState>(startTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(m, m_motionState.get(), m_colShape.get(), localInertia);

	m_rigidBody = std::make_unique<btRigidBody>(rbInfo);

	CollisionManager::instance().addRigidBody(this);
}

CollisionRigidBody::CollisionRigidBody(const glm::vec3& scale, float mass, int group, int mask, const glm::vec3& initialPosition,
	const glm::vec3& initialRotation) :
	m_group(group),
	m_mask(mask)
{
	m_colShape = std::make_unique<btBoxShape>(btVector3(scale.x, scale.y, scale.z));

	btScalar    m(mass);

	bool isDynamic = (mass != 0.f);

	btVector3 localInertia(0, 0, 0);
	if (isDynamic)
		m_colShape->calculateLocalInertia(m, localInertia);

	btTransform startTransform;
	startTransform.setIdentity();
	startTransform.setOrigin(btVector3(initialPosition.x, initialPosition.y, initialPosition.z));
	startTransform.setRotation(btQuaternion(initialRotation.x, initialRotation.y, initialRotation.z));

	m_motionState = std::make_unique<btDefaultMotionState>();
	btRigidBody::btRigidBodyConstructionInfo rbInfo(m, m_motionState.get(), m_colShape.get(), localInertia);

	m_rigidBody = std::make_unique<btRigidBody>(rbInfo);

	CollisionManager::instance().addRigidBody(this);
}

CollisionRigidBody::~CollisionRigidBody() {
	if(m_rigidBody)
		CollisionManager::instance().removeRigidBody(this);
}

void CollisionRigidBody::setOnEnterFunction(const std::function<void(int)>& func) {
	m_onCollisionEnter = func;
	CollisionManager::instance().resetOnCollisionEnterFunction(m_rigidBody.get(), func);
}

void CollisionRigidBody::setOnStayFunction(const std::function<void(int)>& func) {
	m_onCollisionStay = func;
	CollisionManager::instance().resetOnCollisionStayFunction(m_rigidBody.get(), func);
}

void CollisionRigidBody::setOnExitFunction(const std::function<void(int)>& func) {
	m_onCollisionExit = func;
	CollisionManager::instance().resetOnCollisionExitFunction(m_rigidBody.get(), func);
}