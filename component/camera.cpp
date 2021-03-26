#include "camera.h"
#include "renderer.h"
#include "../tools/model.h"

Camera::Camera() {
	m_fov = glm::half_pi<float>();
	m_aspect = 16.0f / 9.0f;
	m_nearPlane = 0.1f;
	m_farPlane = 1000.0f;
	m_layer = LAYER::LAYER0;
}

Camera::Camera(float fov, float aspect, float nearPlane, float farPlane, LAYER layer) {
	m_fov = fov;
	m_aspect = aspect;
	m_nearPlane = nearPlane;
	m_farPlane = farPlane;
	m_layer = layer;
}

Camera::~Camera() {

}

void Camera::execute() {
	Transform* transform = parent->getComponent<Transform>();
	glm::vec3 vecY = glm::normalize(glm::vec3(transform->matrix[1][0], transform->matrix[1][1], transform->matrix[1][2]));
	glm::vec3 vecZ = glm::normalize(glm::vec3(transform->matrix[2][0], transform->matrix[2][1], transform->matrix[2][2]));

	viewMatrix = glm::lookAtLH(transform->position, transform->position + vecZ, vecY);
	projMatrix = glm::perspectiveLH(m_fov, m_aspect, m_nearPlane, m_farPlane);
}

void Camera::frustumTest() {
	glm::mat4 invViewProjMatrix = glm::inverse(projMatrix * viewMatrix);

	//------------手前方向の法線計算------------
	{
		glm::vec4 pos[] = {
			glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f),
			glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
			glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
		};

		for (int i = 0; i < 3; i++) {
			pos[i] = invViewProjMatrix * pos[i];
			pos[i] /= pos[i].w;
		}

		glm::vec3 edge1 = glm::vec3(pos[1]) - glm::vec3(pos[0]);
		glm::vec3 edge2 = glm::vec3(pos[2]) - glm::vec3(pos[1]);

		m_frontNor = glm::normalize(glm::cross(edge1, edge2));
	}

	//------------奥方向の法線計算------------
	{
		glm::vec4 pos[] = {
			glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),
			glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
		};

		for (int i = 0; i < 3; i++) {
			pos[i] = invViewProjMatrix * pos[i];
			pos[i] /= pos[i].w;
		}

		glm::vec3 edge1 = glm::vec3(pos[1]) - glm::vec3(pos[0]);
		glm::vec3 edge2 = glm::vec3(pos[2]) - glm::vec3(pos[1]);

		m_frontNor = glm::normalize(glm::cross(edge1, edge2));
	}

	//------------左方向の法線計算------------
	{
		glm::vec4 pos[] = {
			glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f),
			glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),
			glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
		};

		for (int i = 0; i < 3; i++) {
			pos[i] = invViewProjMatrix * pos[i];
			pos[i] /= pos[i].w;
		}

		glm::vec3 edge1 = glm::vec3(pos[1]) - glm::vec3(pos[0]);
		glm::vec3 edge2 = glm::vec3(pos[2]) - glm::vec3(pos[1]);

		m_frontNor = glm::normalize(glm::cross(edge1, edge2));
	}

	//------------右方向の法線計算------------
	{
		glm::vec4 pos[] = {
			glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
			glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
		};

		for (int i = 0; i < 3; i++) {
			pos[i] = invViewProjMatrix * pos[i];
			pos[i] /= pos[i].w;
		}

		glm::vec3 edge1 = glm::vec3(pos[1]) - glm::vec3(pos[0]);
		glm::vec3 edge2 = glm::vec3(pos[2]) - glm::vec3(pos[1]);

		m_frontNor = glm::normalize(glm::cross(edge1, edge2));
	}

	//------------上方向の法線計算------------
	{
		glm::vec4 pos[] = {
			glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
			glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
			glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
		};

		for (int i = 0; i < 3; i++) {
			pos[i] = invViewProjMatrix * pos[i];
			pos[i] /= pos[i].w;
		}

		glm::vec3 edge1 = glm::vec3(pos[1]) - glm::vec3(pos[0]);
		glm::vec3 edge2 = glm::vec3(pos[2]) - glm::vec3(pos[1]);

		m_frontNor = glm::normalize(glm::cross(edge1, edge2));
	}

	//------------下方向の法線計算------------
	{
		glm::vec4 pos[] = {
			glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f),
			glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
		};

		for (int i = 0; i < 3; i++) {
			pos[i] = invViewProjMatrix * pos[i];
			pos[i] /= pos[i].w;
		}

		glm::vec3 edge1 = glm::vec3(pos[1]) - glm::vec3(pos[0]);
		glm::vec3 edge2 = glm::vec3(pos[2]) - glm::vec3(pos[1]);

		m_frontNor = glm::normalize(glm::cross(edge1, edge2));
	}

	glm::vec4 nearPos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec4 farPos = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

	nearPos = invViewProjMatrix * nearPos;
	farPos = invViewProjMatrix * farPos;

	nearPos /= nearPos.w;
	farPos /= farPos.w;

	std::vector<GameObject*>* objectArray = Scheduler::instance().getObjectPerLayer(m_layer);
	for (auto& ite : (*objectArray)) {
		Transform* trans = ite->getComponent<Transform>();
		MeshRenderer* renderer = ite->getComponent<MeshRenderer>();
		Mesh* mesh = renderer->mesh;
		glm::vec3 size = (mesh->getMax() - mesh->getMin()) * trans->scale;

	}
}