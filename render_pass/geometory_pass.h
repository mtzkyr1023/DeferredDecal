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

private:
	struct CB0 {
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 padding1;
		glm::mat4 padding2;
	};

private:
	Shader m_vs;
	Shader m_ps;

	DescriptorHeap m_cbvHeap;
	DescriptorHeap m_samplerHeap;
	DescriptorHeap m_rtvHeap;
	DescriptorHeap m_dsvHeap;

	ConstantBuffer<CB0> m_cb0;

	Mesh m_mesh;
};


#endif