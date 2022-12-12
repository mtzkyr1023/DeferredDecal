#include "player.h"

#include "../../tools/input.h"

#include "../collision/collision_manager.h"

#include "../../tools/input.h"


Player::Player() {
	m_collision = std::make_unique<CollisionCharacterController>(1.0f, 2.1f, (int)CollisionMask::ePlayer, (int)CollisionMask::eGround | (int)CollisionMask::eEnemy,
		glm::vec3(0.1f, 20.0f, 0.0f), glm::vec3(0.0f, 90.0f, 0.0f), 0.2f);

	m_collision->characterController()->setGravity(btVector3(0.0f, 0.0, 0.0f));

	//m_collision->setOnEnterFunction([this](int mask) {
	//	//m_collision->characterController()->jump(btVector3(0.0f, 10.0f, 0.0f));
	//});
	//m_collision->setOnExitFunction([this](int mask) {
	//	m_collision->characterController()->applyImpulse(btVector3(2.0f, 0.0f, 0.0f));
	//});

	m_renderer = std::make_unique<MeshRenderer>("models/sphere.gltf");

	m_camera = std::make_unique<Camera>(glm::radians(90.0f), 16.0f / 9.0f, 0.1f, 2000.0f);

	m_camera->transform().setParent(&m_transform);
}


Player::~Player() {

}


void Player::update(float deltaTime) {

	Transform& trs = m_camera->transform();

	glm::vec3 moveDirection = glm::vec3(0.0f, 0.0f, 0.0f);

	const float moveSpeed = 60.0f;

	if (Input::Instance().Push(DIK_S)) {
		moveDirection += glm::vec3(-trs.forward().x, -trs.forward().y, -trs.forward().z) * moveSpeed * deltaTime;
	}
	if (Input::Instance().Push(DIK_W)) {
		moveDirection += glm::vec3(trs.forward().x, trs.forward().y, trs.forward().z) * moveSpeed * deltaTime;
	}
	if (Input::Instance().Push(DIK_A)) {
		moveDirection += glm::vec3(-trs.right().x, -trs.right().y, -trs.right().z) * moveSpeed * deltaTime;
	}
	if (Input::Instance().Push(DIK_D)) {
		moveDirection += glm::vec3(trs.right().x, trs.right().y, trs.right().z) * moveSpeed * deltaTime;
	}
	if (Input::Instance().Push(DIK_Q)) {
		moveDirection += glm::vec3(-trs.up().x, -trs.up().y, -trs.up().z) * moveSpeed * deltaTime;
	}
	if (Input::Instance().Push(DIK_E)) {
		moveDirection += glm::vec3(trs.up().x, trs.up().y, trs.up().z) * moveSpeed * deltaTime;
	}

	m_collision->characterController()->setWalkDirection(btVector3(moveDirection.x, moveDirection.y, moveDirection.z));

	if(Input::Instance().Trigger(DIK_SPACE))
		m_collision->characterController()->jump(btVector3(0.0f, 10.0f, 0.0f));


	float x = m_collision->ghostObject()->getWorldTransform().getOrigin().x();
	float y = m_collision->ghostObject()->getWorldTransform().getOrigin().y();
	float z = m_collision->ghostObject()->getWorldTransform().getOrigin().z();

	m_transform.TranslateLocal(x, y, z);

	btScalar bx, by, bz;

	m_collision->ghostObject()->getWorldTransform().getRotation().getEulerZYX(bz, by, bx);

	m_transform.RotateWorld(bx, by, bz);

	m_camera->Update(deltaTime);
}