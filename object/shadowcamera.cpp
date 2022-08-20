#include "shadowcamera.h"

ShadowCamera::ShadowCamera(const glm::vec3& position, const glm::quat& rotation, float width, float height, float nearZ, float farZ) :
	m_viewMatrix(glm::identity<glm::mat4>()),
	m_projMatrix(glm::identity<glm::mat4>()),
	m_width(width),
	m_height(height),
	m_nearZ(nearZ),
	m_farZ(farZ)
{

	m_transform.position() = position;
	m_transform.rotation() = rotation;
}

ShadowCamera::~ShadowCamera() {

}

void ShadowCamera::Update() {

	float depth = m_farZ - m_nearZ;

	m_viewMatrix = glm::lookAt(m_transform.position() - m_transform.forward() * depth * 0.5f, m_transform.position(), m_transform.up());
	m_projMatrix = glm::ortho(-m_width * 0.5f, m_width * 0.5f, -m_height * 0.5f, m_height * 0.5f, -depth, depth);
	//m_projMatrix = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, m_nearZ, m_farZ);
}