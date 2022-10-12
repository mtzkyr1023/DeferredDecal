#pragma once


#include "transform.h"

#include <vector>

class Camera {
public:
	Camera(float fov = glm::radians(90.0f), float aspect = 16.0f / 9.0f, float nearZ = 0.1f, float farZ = 2000.0f);
	~Camera();

	Transform& transform() { return m_transform; }

	const glm::mat4& viewMatrix() { return m_viewMatrix; }
	const glm::mat4& projMatrix() { return m_projMatrix; }

	const std::vector<glm::vec4>& frustumMin() { return m_frustumMin; }
	const std::vector<glm::vec4>& frustumMax() { return m_frustumMax; }

	void Update(float deltaTime = 1.0f / 60.0f);

	static const int FRUSTUM_DIV_SIZE_X = 16;
	static const int FRUSTUM_DIV_SIZE_Y = 16;
	static const int FRUSTUM_DIV_SIZE_Z = 16;

	float& nearZ() { return m_near; }
	float& farZ() { return m_far; }

private:
	glm::vec3 lineIntersectionZPlane(glm::vec3 A, glm::vec3 B, float zPlane);

private:
	Transform m_transform;

	glm::mat4 m_viewMatrix;
	glm::mat4 m_projMatrix;

	float m_fov;
	float m_aspect;
	float m_near;
	float m_far;

	std::vector<glm::vec4> m_frustumMin;
	std::vector<glm::vec4> m_frustumMax;
};