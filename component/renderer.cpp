#include "renderer.h"
#include "../tools/model.h"

Renderer::Renderer() {
	mesh = MeshManager::instance().getMesh(DEFAULT_MESH);
}

void Renderer::setMesh(const char* name) {
	mesh = MeshManager::instance().getMesh(name);
}