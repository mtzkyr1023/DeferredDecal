#ifndef _GEOMETORY_PASS_H_
#define _GEOMETORY_PASS_H_


#include "render_pass.h"

#include "../component/renderer.h"
#include "../component/camera.h"

#include "../tools/model.h"

class GeometoryPass : public RenderPass {
public:
	GeometoryPass() = default;
	~GeometoryPass() = default;

	bool create(ID3D12Device* device, ID3D12CommandQueue* queue, UINT backBufferCount, UINT width, UINT height);

	void render(ID3D12GraphicsCommandList* command, UINT curImageCount);

	void run(UINT curImageCount);

	bool setDescriptorHeap(ID3D12Device* device, UINT backBufferCount);

private:
	Shader m_vs;
	Shader m_ps;

	DescriptorHeap m_cbvHeap;
	DescriptorHeap m_samplerHeap;
	DescriptorHeap m_rtvHeap;
	DescriptorHeap m_dsvHeap;

	std::vector<ConstantBuffer> m_cb1Array;
	std::list<SimpleMeshRenderer*> m_simpleMeshRendererList;

	Mesh m_mesh;
};


#endif