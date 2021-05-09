#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "component.h"
#include "transform.h"

class Camera : public Component {
public:
	Camera();
	Camera(float fov, float aspect, float nearPlane, float farPlane, LAYER layer = LAYER::LAYER0);
	~Camera();
	
	void execute(float deltaTime);

	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;

private:
	void frustumTest();

private:
	LAYER m_layer;
	float m_fov;
	float m_aspect;
	float m_nearPlane;
	float m_farPlane;
	glm::vec3 m_frontNor;
	glm::vec3 m_backNor;
	glm::vec3 m_leftNor;
	glm::vec3 m_rightNor;
	glm::vec3 m_topNor;
	glm::vec3 m_bottomNor;
};

#endif