#include "transform.h"

Transform::Transform() {
	this->position = glm::vec3(0.0f);
	this->rotation = glm::vec3(0.0f);
	this->scale = glm::vec3(1.0f);
	this->quaternion = glm::identity<glm::quat>();
	this->matrix = glm::identity<glm::mat4>();
}

Transform::Transform(glm::vec3 pos, glm::vec3 rotate, glm::vec3 scale) {
	this->position = pos;
	this->rotation = rotate;
	this->scale = scale;
	this->quaternion = glm::quat(rotate);
	this->matrix = glm::translate(glm::identity<glm::mat4>(), pos) * glm::mat4(this->quaternion) * glm::scale(glm::identity<glm::mat4>(), scale);
}

Transform::~Transform() {

}

void Transform::execute() {
	quaternion = glm::quat(glm::radians(rotation));
	matrix = glm::translate(glm::identity<glm::mat4>(), position) * glm::mat4(quaternion) * glm::scale(glm::identity<glm::mat4>(), scale);
}