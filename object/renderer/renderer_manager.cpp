#include "renderer_manager.h"


void RendererManager::registMeshRenderer(MeshRenderer* renderer) {
	m_meshRendererList[renderer->filename()].push_back(renderer);
}

void RendererManager::unregistMeshRenderer(MeshRenderer* renderer) {
	m_meshRendererList[renderer->filename()].remove(renderer);
}