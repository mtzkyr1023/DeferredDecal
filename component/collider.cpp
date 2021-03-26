#include "collider.h"
#include "transform.h"

SphereCollider::SphereCollider() {
	m_radius = 1.0f;
	m_shape = SHAPE::SPHERE;
}

SphereCollider::~SphereCollider() {

}

bool SphereCollider::sphereTest(Collider* collider) {
	SphereCollider* sphere = static_cast<SphereCollider*>(collider);
	Transform* trans1 = parent->getComponent<Transform>();
	Transform* trans2 = sphere->parent->getComponent<Transform>();
	glm::vec3 dis = trans2->position - trans1->position;
	float radius1 = m_radius * glm::max(trans1->scale.x, glm::max(trans1->scale.y, trans1->scale.z));
	float radius2 = sphere->getRaidus() * glm::max(trans2->scale.x, glm::max(trans2->scale.y, trans2->scale.z));

	if (dis.x * dis.x + dis.y * dis.y + dis.z * dis.z < (radius1 + radius2) * (radius1 + radius2))
		return true;

	return false;
}

bool SphereCollider::aabbTest(Collider* collider) {
	return false;
}

bool SphereCollider::obbTest(Collider* collider) {
	return false;
}

bool SphereCollider::meshTest(Collider* collider) {
	return false;
}

void SphereCollider::setRadius(Mesh* mesh) {
	glm::vec3 size = mesh->getMax() - mesh->getMin();
	m_radius = glm::max(size.x, glm::max(size.y, size.z));
}