#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "resource.h"
#include "swapchain.h"

class Texture : public Resource {
public:
	Texture() = default;
	~Texture() = default;

	bool createBackBuffer(ID3D12Device* device, IDXGISwapChain3* swapchain, UINT textureCount);
	bool createRenderTarget2D(ID3D12Device* device, UINT textureCount, D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT format,
		UINT width, UINT height);
	bool createRenderTarget3D(ID3D12Device* device, UINT textureCount, D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT format,
		UINT width, UINT height, UINT depth);
	bool createDepthStencilBuffer(ID3D12Device* device, UINT textureCount, DXGI_FORMAT format,
		UINT width, UINT height);
	bool createResource(ID3D12Device* device, ID3D12CommandQueue* queue, UINT textureCount,
		UINT width, UINT height, UINT componentCount, void* data, bool isMipmap);
	bool createResource(ID3D12Device* device, ID3D12CommandQueue* queue, UINT textureCount, DXGI_FORMAT format,
		const char* filename, bool isMipmap);

	void transitionResource(ID3D12GraphicsCommandList* command, UINT textureNum, D3D12_RESOURCE_BARRIER_FLAGS flag,
		D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
};

#endif