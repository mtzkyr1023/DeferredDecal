#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include "../glm-master/glm/glm.hpp"
#include "../glm-master/glm/gtc/matrix_transform.hpp"
#include "../glm-master/glm/gtc/quaternion.hpp"

#include "component.h"

class Transform : public Component {
public:
	Transform();
	~Transform();

	void execute(float deltaTime);

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	glm::quat quaternion;
	glm::mat4 matrix;
};

#endif