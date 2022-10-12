#ifndef _ENEMY_H_
#define _ENEMY_H_

#include "../transform.h"
#include "../collision/collision_character_controller.h"


class Enemy {
public:
	Enemy();
	~Enemy();

	void update();

	const Transform& transform() { return m_transform; }

	const CollisionCharacterController* controller() { return m_collision.get(); }

private:
	Transform m_transform;

	std::unique_ptr<CollisionCharacterController> m_collision;
};

#endif