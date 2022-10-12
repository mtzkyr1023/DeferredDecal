#ifndef _COLLISION_RIGID_BODY_H_
#define _COLLISION_RIGID_BODY_H_


#include <memory>
#include <functional>

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"

#include "../../glm-master/glm/glm.hpp"
#include "../../glm-master/glm/gtc/matrix_transform.hpp"
#include "../../glm-master/glm/gtc/type_ptr.hpp"

class CollisionRigidBody {
public:
	CollisionRigidBody();
	CollisionRigidBody(float radius, float mass, int group, int mask, const glm::vec3& initialPosition = glm::vec3(0.0f, 0.0f, 0.0f),
		const glm::vec3& rotation = glm::vec3(0.0f, 0.0f, 0.0f));
	CollisionRigidBody(const glm::vec3& scale, float mass, int group, int mask, const glm::vec3& initialPosition = glm::vec3(0.0f, 0.0f, 0.0f),
		const glm::vec3& initialRotation = glm::vec3(0.0f, 0.0f, 0.0f));
	~CollisionRigidBody();

	btRigidBody* rigidBody() { return m_rigidBody.get(); }


	void setOnEnterFunction(const std::function<void(int)>& func);
	void setOnStayFunction(const std::function<void(int)>& func);
	void setOnExitFunction(const std::function<void(int)>& func);

	std::function<void(int)>& onCollisionEnter() { return m_onCollisionEnter; }
	std::function<void(int)>& onCollisionStay() { return m_onCollisionStay; }
	std::function<void(int)>& onCollisionExit() { return m_onCollisionExit; }

	int group() { return m_group; }
	int mask() { return m_mask; }

private:
	std::unique_ptr<btRigidBody> m_rigidBody;
	std::unique_ptr<btCollisionShape> m_colShape;
	std::unique_ptr<btDefaultMotionState> m_motionState;

	std::function<void(int)> m_onCollisionEnter;
	std::function<void(int)> m_onCollisionStay;
	std::function<void(int)> m_onCollisionExit;

	int m_group;
	int m_mask;
};

#endif