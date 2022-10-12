#include "mesh_renderer.h"
#include "renderer_manager.h"

MeshRenderer::MeshRenderer() :
	m_filename("models/cube.gltf") {
	RendererManager::instance().registMeshRenderer(this);
}


MeshRenderer::MeshRenderer(const char* filename) :
	m_filename(filename) {
	RendererManager::instance().registMeshRenderer(this);
}

MeshRenderer::~MeshRenderer() {

}

void MeshRenderer::setFilename(const char* filename) {
	RendererManager::instance().unregistMeshRenderer(this);
	m_filename = filename;
	RendererManager::instance().registMeshRenderer(this);
}