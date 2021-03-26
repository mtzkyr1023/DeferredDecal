#ifndef _COLLIDER_H_
#define _COLLIDER_H_


#include "component.h"
#include "../tools/model.h"

enum class SHAPE {
	SPHERE,
	AABB,
	OBB,
	MESH,
};

class Collider : public Component {
public:
	virtual ~Collider() = default;

	virtual bool sphereTest(Collider* collider) = 0;
	virtual bool aabbTest(Collider* collider) = 0;
	virtual bool obbTest(Collider* collider) = 0;
	virtual bool meshTest(Collider* collider) = 0;

	SHAPE getShape() { return m_shape; }
	
protected:
	SHAPE m_shape;
};

class SphereCollider : public Collider {
public:
	SphereCollider();
	~SphereCollider();

	bool sphereTest(Collider* collider);
	bool aabbTest(Collider* collider);
	bool obbTest(Collider* collider);
	bool meshTest(Collider* collider);

	void execute() {}

	void setRadius(float radius) { m_radius = radius; }
	void setRadius(Mesh* mesh);

	float getRaidus() { return m_radius; }

private:
	float m_radius;
};

//class MeshCollider : public Collider {
//public:
//	MeshCollider();
//	~MeshCollider();
//
//	bool sphereTest(Collider* collider);
//	bool aabbTest(Collider* collider);
//	bool obbTest(Collider* collider);
//	bool meshTest(Collider* collider);
//
//	void execute();
//
//private:
//	std::vector<glm::vec3> m_posArray;
//};

#endif