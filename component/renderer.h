#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "component.h"

class Mesh;

class Renderer : public Component {
public:
	Renderer();
	~Renderer() = default;

	void setMesh(const char* name);

	bool castShadows = false;
	float maxDistance = 1000.0f;
	float maxAngle = 90.0f;
	Mesh* mesh = nullptr;
	int layer = 1;
};

class MeshRenderer : public Renderer {
public:
	MeshRenderer() = default;
	~MeshRenderer() = default;

	void execute() {}
};

#endif