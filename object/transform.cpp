#include "transform.h"


Transform::Transform() :
	m_localPosition(0.0f, 0.0f, 0.0f),
	m_localAngle(0.0f, 0.0f, 0.0f),
	m_localScale(1.0f, 1.0f, 1.0f),
	m_localRotation(glm::identity<glm::quat>()),
	m_position(0.0f, 0.0f, 0.0f),
	m_angle(0.0f, 0.0f, 0.0f),
	m_scale(1.0f, 1.0f, 1.0f),
	m_rotation(glm::identity<glm::quat>()),
	m_localMatrix(glm::identity<glm::mat4>()),
	m_worldMatrix(glm::identity<glm::mat4>()),
	m_forward(0.0f, 0.0 ,1.0f),
	m_up(0.0f, 1.0f, 0.0f),
	m_right(1.0f,0.0f,0.0f)
{

}

Transform::~Transform() {

}

void Transform::AddTranslation(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localPosition.x += x;
		m_localPosition.y += y;
		m_localPosition.z += z;
		m_dirtyFlag = true;
	}
}

void Transform::TranslateLocal(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localPosition.x = x;
		m_localPosition.y = y;
		m_localPosition.z = z;
		m_dirtyFlag = true;
	}
}

void Transform::TranslateWorld(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localPosition.x = x;
		m_localPosition.y = y;
		m_localPosition.z = z;
		m_dirtyFlag = true;
	}
}

void Transform::AddRotation(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localAngle.x += x;
		m_localAngle.y += y;
		m_localAngle.z += z;
		m_dirtyFlag = true;
	}
}

void Transform::RotateLocal(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localAngle.x = x;
		m_localAngle.y = y;
		m_localAngle.z = z;
		m_dirtyFlag = true;
	}
}

void Transform::RotateWorld(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localAngle.x = x;
		m_localAngle.y = y;
		m_localAngle.z = z;
		m_dirtyFlag = true;
	}
}

void Transform::AddScale(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localScale.x += x;
		m_localScale.y += y;
		m_localScale.z += z;
		m_dirtyFlag = true;
	}
}

void Transform::ScalingLocal(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localScale.x = x;
		m_localScale.y = y;
		m_localScale.z = z;
		m_dirtyFlag = true;
	}
}

void Transform::ScalingWorld(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localScale.x = x;
		m_localScale.y = y;
		m_localScale.z = z;
		m_dirtyFlag = true;
	}
}

void Transform::DirtyUpdate() {
	if (m_dirtyFlag) {

		m_localRotation =
			glm::rotate(glm::identity<glm::quat>(), glm::radians(m_localAngle.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::rotate(glm::identity<glm::quat>(), glm::radians(m_localAngle.x), glm::vec3(0.0f, 0.0f, 1.0f));

		m_localMatrix =
			glm::translate(glm::identity<glm::mat4>(), m_localPosition) *
			glm::mat4(m_localRotation) *
			glm::scale(glm::identity<glm::mat4>(), m_localScale);

		m_position = m_localPosition;
		m_angle = m_localAngle;
		m_scale = m_localScale;

		m_rotation = m_localRotation;

		m_worldMatrix = m_localMatrix;

		m_forward = glm::vec3(m_worldMatrix[0][0], m_worldMatrix[0][1], m_worldMatrix[0][2]);
		m_up = glm::vec3(m_worldMatrix[1][0], m_worldMatrix[1][1], m_worldMatrix[1][2]);
		m_right = glm::vec3(m_worldMatrix[2][0], m_worldMatrix[2][1], m_worldMatrix[2][2]);

		m_dirtyFlag = false;
	}
 }


void Transform::SetDirtyFlag() {
	m_dirtyFlag = true;
}