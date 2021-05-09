#include "sample_scene.h"

#include "../component/camera.h"
#include "../component/renderer.h"
#include "../render_pass/render_pass.h"
#include "../tools/input.h"

void SampleScene::init() {
	m_camera = Scheduler::instance().addGameObject(LAYER::LAYER0, "");
	GameObject* camObject = Scheduler::instance().getGameObject(m_camera);
	Transform* trans = camObject->addComponent<Transform>();
	Camera* camera = camObject->addComponent<Camera>();

	m_cube = Scheduler::instance().addGameObject(LAYER::LAYER0, "");
	GameObject* cubeObject = Scheduler::instance().getGameObject(m_cube);
	SimpleMeshRenderer* meshRenderer = cubeObject->addComponent<SimpleMeshRenderer>();
	trans = cubeObject->addComponent<Transform>();
	meshRenderer->setMesh("models/cube.gltf");
}

void SampleScene::run(uint32_t curImageIndex, float deltaTime) {
	GameObject* camObject = Scheduler::instance().getGameObject(m_camera);
	Camera* camera = camObject->getComponent<Camera>();
	Transform* trans = camObject->getComponent<Transform>();

	const glm::mat4& pose = trans->matrix;
	glm::vec3 forward = glm::vec3(pose[2][0], pose[2][1], pose[2][2]);
	glm::vec3 sideward = glm::vec3(pose[0][0], pose[0][1], pose[0][2]);
	glm::vec3 up = glm::vec3(pose[1][0], pose[1][1], pose[1][2]);

	if (Input::Instance().Push(DIK_W))
		trans->position += forward * deltaTime;
	if (Input::Instance().Push(DIK_S))
		trans->position -= forward * deltaTime;
	if (Input::Instance().Push(DIK_E))
		trans->position += up * deltaTime;
	if (Input::Instance().Push(DIK_Q))
		trans->position -= up * deltaTime;
	if (Input::Instance().Push(DIK_D))
		trans->position += sideward * deltaTime;
	if (Input::Instance().Push(DIK_A))
		trans->position -= sideward * deltaTime;

	trans->rotation.x += Input::Instance().GetMoveYRightPushed();
	trans->rotation.y -= Input::Instance().GetMoveXRightPushed();

	ConstantBuffer* cb0 = ResourceManager::Instance().getResourceAsCB(VIEW_PROJ_BUFFER);
	RenderPass::ViewProjBuffer viewProjBuffer{};
	viewProjBuffer.view = glm::transpose(camera->viewMatrix);
	viewProjBuffer.proj = glm::transpose(camera->projMatrix);

	cb0->updateBuffer(curImageIndex, sizeof(glm::mat4) * 4, &viewProjBuffer);
}