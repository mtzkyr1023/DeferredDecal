#ifndef _RENDER_PASS_H_
#define _RENDER_PASS_H_

#include "../framework/device.h"
#include "../framework/commandbuffer.h"
#include "../framework/descriptor_heap.h"
#include "../framework/pipeline.h"
#include "../framework/queue.h"
#include "../framework/root_signature.h"
#include "../framework/shader.h"
#include "../framework/buffer.h"
#include "../framework/texture.h"
#include "../framework/fence.h"

#include <unordered_map>
#include <memory>

static const char* BACK_BUFFER = "back_buffer";
static const char* ALBEDO_BUFFER = "albedo_bufer";
static const char* NORMAL_BUFFER = "normal_buffer";
static const char* PBR_BUFFER = "pbr_buffer";
static const char* DEPTH_BUFFER = "depth_buffer";
static const char* SAMPLE_TEXTURE = "sample_texture";

class RenderPass {
public:
	virtual ~RenderPass() = default;

	virtual void render(ID3D12GraphicsCommandList* command, UINT curImageCount) = 0;

	virtual void run(UINT curImageCount) = 0;

protected:
	RootSignature m_rootSignature;
	Pipeline m_pipeline;
	UINT m_width;
	UINT m_height;
};


class ResourceManager {
private:
	ResourceManager() = default;
	~ResourceManager() = default;

public:
	static ResourceManager& Instance() {
		static ResourceManager instance;
		return instance;
	}

	bool createBackBuffer(const char* name, ID3D12Device* device, IDXGISwapChain3* swapchain, UINT backBufferCount);
	bool createDepthStencilBuffer(const char* name, ID3D12Device* device, UINT textureCount, UINT width, UINT height, bool isStencil);
	bool createRenderTarget2D(const char* name, ID3D12Device* device, UINT resourceCount, D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT format, UINT width, UINT height);
	bool createTexture(const char* name, ID3D12Device* device, ID3D12CommandQueue* queue, UINT resourceCount, DXGI_FORMAT format,
		const char* filename, bool isMipmap);

	Resource* getResource(const char* name) { return m_resourceArray[name].get(); }

private:
	std::unordered_map<std::string, std::unique_ptr<Resource>> m_resourceArray;
};

#endif