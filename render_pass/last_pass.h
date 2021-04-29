#ifndef _LAST_PASS_H_
#define _LAST_PASS_H_


#include "render_pass.h"

class LastPass : public RenderPass {
public:
	LastPass() = default;
	~LastPass() = default;

	bool create(ID3D12Device* device, UINT backBufferCount, UINT width, UINT height);

	void render(ID3D12GraphicsCommandList* command, UINT curImageCount);

	void run(UINT curImageCount);

private:
	Shader m_vs;
	Shader m_ps;
	DescriptorHeap m_srvHeap;
	DescriptorHeap m_samplerHeap;
	DescriptorHeap m_rtvHeap;
};

#endif