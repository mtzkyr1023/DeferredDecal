#include "transform.h"

Transform::Transform() {
	this->position = glm::vec3(0.0f);
	this->rotation = glm::vec3(0.0f);
	this->scale = glm::vec3(1.0f);
	this->quaternion = glm::identity<glm::quat>();
	this->matrix = glm::identity<glm::mat4>();
}

Transform::~Transform() {

}

void Transform::execute(float deltaTime) {
	quaternion = glm::quat(glm::radians(rotation));
	matrix = glm::translate(glm::identity<glm::mat4>(), position) * glm::mat4(quaternion) * glm::scale(glm::identity<glm::mat4>(), scale);
}