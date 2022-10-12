#ifndef _COLLISION_RESOURCE_H_
#define _COLLISION_RESOURCE_H_


#include "collision_manager.h"

class CollisionResource {
public:

	CollisionResource();
	~CollisionResource();

	virtual bool createShape(btCollisionShape* shape) = 0;
	virtual void destroy() = 0;

protected:

	std::unique_ptr<btCollisionObject> m_collisionObject;
};

#endif