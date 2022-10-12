#ifndef _SHADOW_CAMERA_H_
#define _SHADOW_CAMERA_H_


#include "transform.h"


class ShadowCamera {
public:
	ShadowCamera(const glm::vec3& position, const glm::vec3& rotation, float width, float height, float nearZ  = 0.1f, float farZ = 1000.0f);
	~ShadowCamera();

	void Update();

	const glm::mat4& viewMatrix() { return m_viewMatrix; }
	const glm::mat4& projMatrix() { return m_projMatrix; }

	Transform& transform() { return m_transform; }

private:
	glm::mat4 m_viewMatrix;
	glm::mat4 m_projMatrix;

	float m_width;
	float m_height;

	float m_nearZ;
	float m_farZ;

	Transform m_transform;
};

#endif