#pragma once


#include <vector>
#include "../glm-master/glm/glm.hpp"
#include "../glm-master/glm/gtc/matrix_transform.hpp"
#include "../glm-master/glm/gtc/type_ptr.hpp"


class Transform {
public:
	Transform();
	~Transform();

	void AddTranslation(float x, float y, float z);
	void TranslateLocal(float x, float y, float z);
	void TranslateWorld(float x, float y, float z);

	void AddRotation(float x, float y, float z);
	void RotateLocal(float x, float y, float z);
	void RotateWorld(float x, float y, float z);

	void AddScale(float x, float y, float z);
	void ScalingLocal(float x, float y, float z);
	void ScalingWorld(float x, float y, float z);

	glm::vec3& localPosition() { DirtyUpdate(); return m_localPosition; }
	glm::vec3& localAngle() { DirtyUpdate();  return m_localAngle; }
	glm::vec3& localScale() { DirtyUpdate();  return m_localScale; }
	glm::quat& localRotation() { DirtyUpdate();  return m_localRotation; }
	
	glm::vec3& position() { DirtyUpdate();  return m_position; }
	glm::vec3& angle() { DirtyUpdate();  return m_angle; }
	glm::vec3& scale() { DirtyUpdate();  return m_scale; }
	glm::quat& rotation() { DirtyUpdate();  return m_rotation; }

	glm::mat4& localMatrix() { DirtyUpdate(); return m_localMatrix; }
	glm::mat4& worldMatrix() { DirtyUpdate(); return m_worldMatrix; }

	const glm::vec3& forward() { DirtyUpdate();  return m_forward; }
	const glm::vec3& right() { DirtyUpdate();  return m_right; }
	const glm::vec3& up() { DirtyUpdate(); return m_up; }

private:
	void SetDirtyFlag();
	void DirtyUpdate();

	glm::vec3 m_localPosition;
	glm::vec3 m_localAngle;
	glm::vec3 m_localScale;
	glm::quat m_localRotation;

	glm::vec3 m_position;
	glm::vec3 m_angle;
	glm::vec3 m_scale;
	glm::quat m_rotation;

	glm::vec3 m_forward;
	glm::vec3 m_right;
	glm::vec3 m_up;

	glm::mat4 m_localMatrix;
	glm::mat4 m_worldMatrix;
	
	bool m_dirtyFlag;
};