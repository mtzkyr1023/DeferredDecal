#include "transform.h"


Transform::Transform() :
	m_localPosition(0.0f, 0.0f, 0.0f),
	m_localScale(1.0f, 1.0f, 1.0f),
	m_localRotation(glm::identity<glm::quat>()),
	m_position(0.0f, 0.0f, 0.0f),
	m_scale(1.0f, 1.0f, 1.0f),
	m_rotation(glm::identity<glm::quat>()),
	m_localMatrix(glm::identity<glm::mat4>()),
	m_worldMatrix(glm::identity<glm::mat4>()),
	m_forward(0.0f, 0.0, 1.0f),
	m_up(0.0f, 1.0f, 0.0f),
	m_right(1.0f, 0.0f, 0.0f),
	m_parent(nullptr),
	m_dirtyFlag(true)
{

}

Transform::~Transform() {

}

void Transform::AddTranslation(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localPosition.x += x;
		m_localPosition.y += y;
		m_localPosition.z += z;
		SetDirtyFlag();
	}
}

void Transform::TranslateLocal(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localPosition.x = x;
		m_localPosition.y = y;
		m_localPosition.z = z;
		SetDirtyFlag();
	}
}

void Transform::TranslateWorld(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localPosition.x = x;
		m_localPosition.y = y;
		m_localPosition.z = z;
		SetDirtyFlag();
	}
}

void Transform::AddRotation(float x, float y, float z) {
	m_localRotation = m_localRotation *
		glm::rotate(glm::identity<glm::quat>(), x, glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::rotate(glm::identity<glm::quat>(), y, glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::rotate(glm::identity<glm::quat>(), z, glm::vec3(0.0f, 0.0f, 1.0f));
	SetDirtyFlag();
}

void Transform::RotateLocal(float x, float y, float z) {
	m_localRotation = m_localRotation *
		glm::rotate(glm::identity<glm::quat>(), x, glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::rotate(glm::identity<glm::quat>(), y, glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::rotate(glm::identity<glm::quat>(), z, glm::vec3(0.0f, 0.0f, 1.0f));
	SetDirtyFlag();
}

void Transform::RotateWorld(float x, float y, float z) {
	m_localRotation = glm::quat(glm::vec3(x, y, z));
	SetDirtyFlag();
}

void Transform::AddScale(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localScale.x += x;
		m_localScale.y += y;
		m_localScale.z += z;
		SetDirtyFlag();
	}
}

void Transform::ScalingLocal(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localScale.x = x;
		m_localScale.y = y;
		m_localScale.z = z;
		SetDirtyFlag();
	}
}

void Transform::ScalingWorld(float x, float y, float z) {
	if (x != 0 || y != 0 || z != 0) {
		m_localScale.x = x;
		m_localScale.y = y;
		m_localScale.z = z;
		SetDirtyFlag();
	}
}

void Transform::DirtyUpdate() {
	if (m_dirtyFlag) {

		m_localMatrix =
			glm::translate(glm::identity<glm::mat4>(), m_localPosition) *
			glm::mat4(m_localRotation) *
			glm::scale(glm::identity<glm::mat4>(), m_localScale);


		if (m_parent != nullptr) {
			m_worldMatrix = m_localMatrix * m_parent->worldMatrix();
		}
		else {
			m_worldMatrix = m_localMatrix;
		}

		m_position = glm::vec3(m_worldMatrix[3][0], m_worldMatrix[3][1], m_worldMatrix[3][2]);
		m_rotation = glm::quat(m_worldMatrix);
		m_scale = glm::vec3(m_worldMatrix[0][0], m_worldMatrix[1][1], m_worldMatrix[2][2]);

		glm::mat4 pose(m_rotation);

		m_forward = glm::vec3(pose[0][0], pose[0][1], pose[0][2]);
		m_up = glm::vec3(pose[1][0], pose[1][1], pose[1][2]);
		m_right = glm::vec3(pose[2][0], pose[2][1], pose[2][2]);

		m_forward = glm::vec3(m_localMatrix[0][0], m_localMatrix[0][1], m_localMatrix[0][2]);
		m_up = glm::vec3(m_localMatrix[1][0], m_localMatrix[1][1], m_localMatrix[1][2]);
		m_right = glm::vec3(m_localMatrix[2][0], m_localMatrix[2][1], m_localMatrix[2][2]);

		m_dirtyFlag = false;
	}
 }


void Transform::SetDirtyFlag() {
	m_dirtyFlag = true;
	for (auto& ite : m_children) {
		ite->SetDirtyFlag();
	}
}