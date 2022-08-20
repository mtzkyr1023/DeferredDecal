#include "camera.h"
#include "../tools/input.h"

Camera::Camera() :
	m_viewMatrix(glm::identity<glm::mat4>()),
	m_projMatrix(glm::identity<glm::mat4>())
{

}

Camera::~Camera() {

}

void Camera::Update() {
	if (Input::Instance().Push(DIK_S)) {
		m_transform.AddTranslation(m_transform.forward().x, m_transform.forward().y, m_transform.forward().z);
	}
	if (Input::Instance().Push(DIK_W)) {
		m_transform.AddTranslation(-m_transform.forward().x, -m_transform.forward().y, -m_transform.forward().z);
	}
	if (Input::Instance().Push(DIK_A)) {
		m_transform.AddTranslation(m_transform.right().x, m_transform.right().y, m_transform.right().z);
	}
	if (Input::Instance().Push(DIK_D)) {
		m_transform.AddTranslation(-m_transform.right().x, -m_transform.right().y, -m_transform.right().z);
	}
	if (Input::Instance().Push(DIK_E)) {
		m_transform.AddTranslation(m_transform.up().x, m_transform.up().y, m_transform.up().z);
	}
	if (Input::Instance().Push(DIK_Q)) {
		m_transform.AddTranslation(-m_transform.up().x, -m_transform.up().y, -m_transform.up().z);
	}

	
	m_transform.AddRotation((float)Input::Instance().GetMoveYRightPushed() * 0.1f, (float)Input::Instance().GetMoveXRightPushed() * 0.1f, 0.0f);

	m_viewMatrix = glm::lookAtLH(m_transform.position(), m_transform.position() + m_transform.forward(), m_transform.up());
	m_projMatrix = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 10000.0f);
}