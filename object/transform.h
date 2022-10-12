#pragma once


#include <list>
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
	glm::vec3& localScale() { DirtyUpdate();  return m_localScale; }
	glm::quat& localRotation() { DirtyUpdate();  return m_localRotation; }
	
	glm::vec3& position() { DirtyUpdate();  return m_position; }
	glm::vec3& scale() { DirtyUpdate();  return m_scale; }
	glm::quat& rotation() { DirtyUpdate();  return m_rotation; }

	glm::mat4& localMatrix() { DirtyUpdate(); return m_localMatrix; }
	glm::mat4& worldMatrix() { DirtyUpdate(); return m_worldMatrix; }

	const glm::vec3& forward() { DirtyUpdate();  return m_forward; }
	const glm::vec3& right() { DirtyUpdate();  return m_right; }
	const glm::vec3& up() { DirtyUpdate(); return m_up; }

	const glm::vec3& localForward() { DirtyUpdate();  return m_localForward; }
	const glm::vec3& localRight() { DirtyUpdate();  return m_localRight; }
	const glm::vec3& localUp() { DirtyUpdate(); return m_localUp; }

	void setParent(Transform* parent) { m_parent = parent; parent->addChild(this); }
	void addChild(Transform* child) { m_children.push_back(child); }

	Transform* parent() { return m_parent; }
	const std::list<Transform*>& children() { return m_children; }

public:
	void SetDirtyFlag();
	void DirtyUpdate();

private:

	glm::vec3 m_localPosition;
	glm::vec3 m_localScale;
	glm::quat m_localRotation;

	glm::vec3 m_position;
	glm::vec3 m_scale;
	glm::quat m_rotation;

	glm::vec3 m_localRight;
	glm::vec3 m_localUp;
	glm::vec3 m_localForward;

	glm::vec3 m_forward;
	glm::vec3 m_right;
	glm::vec3 m_up;

	glm::mat4 m_localMatrix;
	glm::mat4 m_worldMatrix;

	Transform* m_parent;
	std::list<Transform*> m_children;
	
	bool m_dirtyFlag;
};