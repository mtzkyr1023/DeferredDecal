#ifndef _GEOMETORY_PASS_H_
#define _GEOMETORY_PASS_H_


#include "render_pass.h"

class GeometoryPass : public RenderPass {
public:
	GeometoryPass() = default;
	~GeometoryPass() = default;

	bool create(ID3D12Device* device, ID3D12CommandQueue* queue, UINT backBufferCount, UINT width, UINT height);

	void render(ID3D12GraphicsCommandList* command, UINT curImageCount);

	void run(UINT curImageCount);

private:

};


#endif