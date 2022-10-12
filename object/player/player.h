#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "../transform.h"
#include "../collision/collision_character_controller.h"
#include "../renderer/mesh_renderer.h"
#include "../../object/camera.h"

class Player {
public:
	Player();
	~Player();

	void update(float deltaTime = 1.0f / 60.0f);

	Transform& transform() { return m_transform; }

	Camera* camera() { return m_camera.get(); }

private:
	Transform m_transform;
	std::unique_ptr<CollisionCharacterController> m_collision;
	std::unique_ptr<MeshRenderer> m_renderer;

	std::unique_ptr<Camera> m_camera;
};

#endif