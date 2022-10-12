#ifndef _COLLISIN_GHOST_OBJECT_H_
#define _COLLISIN_GHOST_OBJECT_H_

#include <memory>
#include <functional>

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"


#include "../../glm-master/glm/glm.hpp"
#include "../../glm-master/glm/gtc/matrix_transform.hpp"
#include "../../glm-master/glm/gtc/type_ptr.hpp"

class CollisionGhostObject {
public:
	CollisionGhostObject();
	CollisionGhostObject(float radius, float height, int group, int mask, const glm::vec3& position = glm::vec3(), const glm::vec3& rotation = glm::vec3(0.0f, 0.0f, 0.0f));
	~CollisionGhostObject();

	btGhostObject* ghostObject() { return m_ghostObject.get(); }


	void setOnEnterFunction(const std::function<void(int)>& func);
	void setOnStayFunction(const std::function<void(int)>& func);
	void setOnExitFunction(const std::function<void(int)>& func);

	std::function<void(int)>& onCollisionEnter() { return m_onCollisionEnter; }
	std::function<void(int)>& onCollisionStay() { return m_onCollisionStay; }
	std::function<void(int)>& onCollisionExit() { return m_onCollisionExit; }

	int group() { return m_group; }
	int mask() { return m_mask; }

private:
	std::unique_ptr<btGhostObject>  m_ghostObject;
	std::unique_ptr<btCollisionShape> m_colShape;

	std::function<void(int)> m_onCollisionEnter;
	std::function<void(int)> m_onCollisionStay;
	std::function<void(int)> m_onCollisionExit;

	int m_group;
	int m_mask;
};


#endif