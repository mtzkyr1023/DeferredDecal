#ifndef _CUBECAMERA_H_
#define _CUBECAMERA_H_

#include "transform.h"

class CubeCamera {
public:
	CubeCamera(const glm::vec3& position);
	~CubeCamera();

	Transform& transform(int index) { return m_transforms[index]; }

	glm::mat4& viewMatrix(int index) { return m_viewMatrix[index]; }
	glm::mat4& projMatrix(int index) { return m_projMatrix[index]; }
	
	void Update();

private:

	static const int PLANE_NUM = 6;
	
	Transform m_transforms[PLANE_NUM];
	
	glm::mat4 m_viewMatrix[PLANE_NUM];
	glm::mat4 m_projMatrix[PLANE_NUM];
};

#endif