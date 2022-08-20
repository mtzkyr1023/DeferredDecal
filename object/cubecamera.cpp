#include "cubecamera.h"


CubeCamera::CubeCamera(const glm::vec3& position) {

	for (int i = 0; i < PLANE_NUM; i++) {
		m_transforms[i].position() = position;
		m_projMatrix[i] = glm::perspective(glm::pi<float>() / 2.0f, 1.0f, 0.1f, 10000.0f);
	}

	m_viewMatrix[0] = glm::lookAtLH(m_transforms[0].position(), m_transforms[0].position() + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_viewMatrix[1] = glm::lookAtLH(m_transforms[1].position(), m_transforms[1].position() - glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_viewMatrix[2] = glm::lookAtLH(m_transforms[2].position(), m_transforms[2].position() - glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	m_viewMatrix[3] = glm::lookAtLH(m_transforms[3].position(), m_transforms[3].position() + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	m_viewMatrix[4] = glm::lookAtLH(m_transforms[4].position(), m_transforms[4].position() - glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_viewMatrix[5] = glm::lookAtLH(m_transforms[5].position(), m_transforms[5].position() + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

CubeCamera::~CubeCamera() {

}


void CubeCamera::Update() {
	m_viewMatrix[0] = glm::lookAtLH(m_transforms[0].position(), m_transforms[0].position() + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_viewMatrix[1] = glm::lookAtLH(m_transforms[1].position(), m_transforms[1].position() - glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_viewMatrix[2] = glm::lookAtLH(m_transforms[2].position(), m_transforms[2].position() - glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	m_viewMatrix[3] = glm::lookAtLH(m_transforms[3].position(), m_transforms[3].position() + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	m_viewMatrix[4] = glm::lookAtLH(m_transforms[4].position(), m_transforms[4].position() - glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_viewMatrix[5] = glm::lookAtLH(m_transforms[5].position(), m_transforms[5].position() + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}