#ifndef _RENDERER_MANAGER_H_
#define _RENDERER_MANAGER_H_


#include "mesh_renderer.h"
#include <memory>
#include <list>
#include <unordered_map>

class RendererManager {
private:
	RendererManager() = default;
	~RendererManager() = default;

public:
	static RendererManager& instance() {
		static RendererManager inst;
		return inst;
	}

	void registMeshRenderer(MeshRenderer* renderer);
	void unregistMeshRenderer(MeshRenderer* renderer);

	const std::unordered_map<std::string, std::list<MeshRenderer*>>& meshRendererList() { return m_meshRendererList; }

private:
	std::unordered_map<std::string, std::list<MeshRenderer*>> m_meshRendererList;
};

#endif