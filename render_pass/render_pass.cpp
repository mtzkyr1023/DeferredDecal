#include "render_pass.h"


bool ResourceManager::createBackBuffer(const char* name, ID3D12Device* device, IDXGISwapChain3* swapchain, UINT backBufferCount) {
	m_resourceArray[name] = std::make_unique<Texture>();
	if (!static_cast<Texture*>(m_resourceArray[name].get())->createBackBuffer(device, swapchain, backBufferCount))
		return false;

	return true;
}


bool ResourceManager::createDepthStencilBuffer(const char* name, ID3D12Device* device, UINT textureCount, UINT width, UINT height, bool isStencil) {
	m_resourceArray[name] = std::make_unique<Texture>();
	if (isStencil) {
		if (!static_cast<Texture*>(m_resourceArray[name].get())->createDepthStencilBuffer(device, textureCount, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, width, height))
			return false;
	}
	else {
		if (!static_cast<Texture*>(m_resourceArray[name].get())->createDepthStencilBuffer(device, textureCount, DXGI_FORMAT_R32_FLOAT, width, height))
			return false;
	}

	return true;
}


bool ResourceManager::createRenderTarget2D(const char* name, ID3D12Device* device, UINT resourceCount, D3D12_RESOURCE_FLAGS flags,
	DXGI_FORMAT format, UINT width, UINT height) {
	m_resourceArray[name] = std::make_unique<Texture>();
	if (!static_cast<Texture*>(m_resourceArray[name].get())->createRenderTarget2D(device, resourceCount, flags, format, width, height))
		return false;

	return true;
}

bool ResourceManager::createTexture(const char* name, ID3D12Device* device, ID3D12CommandQueue* queue, UINT resourceCount, DXGI_FORMAT format,
	const char* filename, bool isMipmap) {
	m_resourceArray[name] = std::make_unique<Texture>();
	if (!static_cast<Texture*>(m_resourceArray[name].get())->createResource(device, queue, resourceCount, format, filename, isMipmap))
		return false;

	return true;
}

bool ResourceManager::createConstantBuffer(const char* name, ID3D12Device* device, UINT size, UINT backBufferCount) {
	m_resourceArray[name] = std::make_unique<ConstantBuffer>();
	if (!static_cast<ConstantBuffer*>(m_resourceArray[name].get())->create(device, size, backBufferCount))
		return false;

	return true;
}