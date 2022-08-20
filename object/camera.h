#pragma once


#include "transform.h"


class Camera {
public:
	Camera();
	~Camera();

	Transform& transform() { return m_transform; }

	const glm::mat4& viewMatrix() { return m_viewMatrix; }
	const glm::mat4& projMatrix() { return m_projMatrix; }

	void Update();

private:
	Transform m_transform;

	glm::mat4 m_viewMatrix;
	glm::mat4 m_projMatrix;
};