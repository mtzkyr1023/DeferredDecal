#include "texture.h"
#include "commandbuffer.h"

#include "../tools/stb_image.h"

#define STB_IMAGE_IMPLEMENTATION


bool Texture::createBackBuffer(ID3D12Device* device, IDXGISwapChain3* swapchain, UINT textureCount) {
	HRESULT res;

	m_resource.resize(textureCount);

	for (UINT i = 0; i < textureCount; i++) {
		res = swapchain->GetBuffer(i, IID_PPV_ARGS(m_resource[i].ReleaseAndGetAddressOf()));
		if (FAILED(res))
			return false;
	}

	return true;
}

bool Texture::createRenderTarget2D(ID3D12Device* device, UINT textureCount, D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT format,
	UINT width, UINT height) {
	HRESULT res;

	m_resource.resize(textureCount);

	for (UINT i = 0; i < textureCount; i++) {
		D3D12_HEAP_PROPERTIES heapProp{};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resDesc{};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0;
		resDesc.Width = width;
		resDesc.Height = height;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 0;
		resDesc.Format = format;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Flags = flags;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format = format;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 0.0f;

		res = device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COMMON,
			&clearValue, IID_PPV_ARGS(m_resource[i].ReleaseAndGetAddressOf()));
		if (FAILED(res)) {
			return false;
		}
	}

	return true;
}

bool Texture::createRenderTarget3D(ID3D12Device* device, UINT textureCount, D3D12_RESOURCE_FLAGS flag, DXGI_FORMAT format,
	UINT width, UINT height, UINT depth) {
	HRESULT res;

	m_resource.resize(textureCount);

	for (UINT i = 0; i < textureCount; i++) {
		D3D12_HEAP_PROPERTIES heapProp{};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resDesc{};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		resDesc.Alignment = 0;
		resDesc.Width = width;
		resDesc.Height = height;
		resDesc.DepthOrArraySize = depth;
		resDesc.MipLevels = 0;
		resDesc.Format = format;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Flags = flag;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		res = device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COMMON,
			nullptr, IID_PPV_ARGS(m_resource[i].ReleaseAndGetAddressOf()));
		if (FAILED(res)) {
			return false;
		}
	}

	return true;
}

bool Texture::createDepthStencilBuffer(ID3D12Device* device, UINT textureCount, DXGI_FORMAT format,
	UINT width, UINT height) {
	HRESULT res;

	m_resource.resize(textureCount);

	for (UINT i = 0; i < textureCount; i++) {
		D3D12_HEAP_PROPERTIES heapProp{};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resDesc{};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0;
		resDesc.Width = width;
		resDesc.Height = height;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 0;
		resDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		res = device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COMMON,
			&clearValue, IID_PPV_ARGS(m_resource[i].ReleaseAndGetAddressOf()));
		if (FAILED(res)) {
			return false;
		}
	}

	return true;
}

bool Texture::createResource(ID3D12Device* device, ID3D12CommandQueue* queue, UINT textureCount,
	UINT width, UINT height, UINT componentCount, void* data, bool isMipmap) {
	HRESULT res;

	m_resource.resize(textureCount);

	CommandAllocator commandAlloc;
	CommandList commandList;
	if (!commandAlloc.createGraphicsCommandAllocator(device))
		return false;
	if (!commandList.createGraphicsCommandList(device, commandAlloc.getCommandAllocator()))
		return false;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTexture2D{};
	UINT numRows = 0;
	UINT64 rowSizeInByte = 0;
	UINT64 requireSize = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> stagingBuffer;
	{
		UINT w = width - 1;
		w |= w >> 1;
		w |= w >> 2;
		w |= w >> 4;
		w |= w >> 8;
		w |= w >> 16;
		w++;
		UINT h = height - 1;
		h |= h >> 1;
		h |= h >> 2;
		h |= h >> 4;
		h |= h >> 8;
		h |= h >> 16;
		h++;
		//w = width;
		//h = height;
		D3D12_RESOURCE_DESC resDesc{};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0;
		resDesc.Width = w;
		resDesc.Height = h;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		device->GetCopyableFootprints(&resDesc, 0, 1, 0, &placedTexture2D, &numRows, &rowSizeInByte, &requireSize);

		D3D12_HEAP_PROPERTIES heapProp{};
		heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;

		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Alignment = 0;
		resDesc.Width = requireSize;
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		res = device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(stagingBuffer.ReleaseAndGetAddressOf()));
		if (FAILED(res))
			return false;

		BYTE* mappedData;
		stagingBuffer->Map(0, nullptr, (void**)(&mappedData));

		for (UINT y = 0; y < (UINT)height; y++) {
			auto dst = mappedData + y * rowSizeInByte;
			auto src = (BYTE*)data + y * width * sizeof(DWORD);
			memcpy_s(dst, width * sizeof(DWORD), src, width * sizeof(DWORD));
		}

		stagingBuffer->Unmap(0, nullptr);
	}

	for (UINT i = 0; i < textureCount; i++) {
		D3D12_HEAP_PROPERTIES heapProp{};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resDesc{};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0;
		resDesc.Width = placedTexture2D.Footprint.Width;
		resDesc.Height = placedTexture2D.Footprint.Height;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		res = device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(m_resource[i].ReleaseAndGetAddressOf()));
		if (FAILED(res))
			return false;

		ID3D12GraphicsCommandList* command = commandList.getCommandList();

		commandAlloc.getCommandAllocator()->Reset();
		command->Reset(commandAlloc.getCommandAllocator(), nullptr);


		D3D12_TEXTURE_COPY_LOCATION dst{};
		D3D12_TEXTURE_COPY_LOCATION src{};
		dst.pResource = m_resource[i].Get();
		dst.SubresourceIndex = 0;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.pResource = stagingBuffer.Get();
		src.PlacedFootprint = placedTexture2D;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		command->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		this->transitionResource(command, i, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		command->Close();

		ID3D12CommandList* cmd[] = {
			command,
		};

		queue->ExecuteCommandLists(1, cmd);

		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		HANDLE fenceEvent = CreateEvent(0, FALSE, FALSE, 0);
		UINT64 fenceValue = 1;

		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.ReleaseAndGetAddressOf()));

		queue->Signal(fence.Get(), fenceValue);

		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);

		CloseHandle(fenceEvent);
	}

	return true;
}


bool Texture::createResource(ID3D12Device* device, ID3D12CommandQueue* queue, UINT textureCount, DXGI_FORMAT format,
	const char* filename, bool isMipmap) {
	HRESULT res;

	m_resource.resize(textureCount);

	CommandAllocator commandAlloc;
	CommandList commandList;
	if (!commandAlloc.createGraphicsCommandAllocator(device))
		return false;
	if (!commandList.createGraphicsCommandList(device, commandAlloc.getCommandAllocator()))
		return false;

	unsigned char* pixels;
	int width, height, bpp;

	pixels = stbi_load(filename, &width, &height, &bpp, 4);

	if (!pixels)
		return false;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTexture2D{};
	UINT numRows = 0;
	UINT64 rowSizeInByte = 0;
	UINT64 requireSize = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> stagingBuffer;
	{
		UINT w = width - 1;
		w |= w >> 1;
		w |= w >> 2;
		w |= w >> 4;
		w |= w >> 8;
		w |= w >> 16;
		w++;
		UINT h = height - 1;
		h |= h >> 1;
		h |= h >> 2;
		h |= h >> 4;
		h |= h >> 8;
		h |= h >> 16;
		h++;
		//w = width;
		//h = height;
		D3D12_RESOURCE_DESC resDesc{};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0;
		resDesc.Width = w;
		resDesc.Height = h;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = format;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		device->GetCopyableFootprints(&resDesc, 0, 1, 0, &placedTexture2D, &numRows, &rowSizeInByte, &requireSize);

		D3D12_HEAP_PROPERTIES heapProp{};
		heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;

		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Alignment = 0;
		resDesc.Width = requireSize;
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		res = device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(stagingBuffer.ReleaseAndGetAddressOf()));
		if (FAILED(res))
			return false;

		BYTE* mappedData;
		stagingBuffer->Map(0, nullptr, (void**)(&mappedData));

		for (UINT y = 0; y < (UINT)height; y++) {
			auto dst = mappedData + y * rowSizeInByte;
			auto src = pixels + y * width * sizeof(DWORD);
			memcpy_s(dst, width * sizeof(DWORD), src, width * sizeof(DWORD));
		}

		stagingBuffer->Unmap(0, nullptr);
	}

	for (UINT i = 0; i < textureCount; i++) {
		D3D12_HEAP_PROPERTIES heapProp{};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resDesc{};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0;
		resDesc.Width = placedTexture2D.Footprint.Width;
		resDesc.Height = placedTexture2D.Footprint.Height;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = format;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		res = device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(m_resource[i].ReleaseAndGetAddressOf()));
		if (FAILED(res))
			return false;

		ID3D12GraphicsCommandList* command = commandList.getCommandList();

		commandAlloc.getCommandAllocator()->Reset();
		command->Reset(commandAlloc.getCommandAllocator(), nullptr);


		D3D12_TEXTURE_COPY_LOCATION dst{};
		D3D12_TEXTURE_COPY_LOCATION src{};
		dst.pResource = m_resource[i].Get();
		dst.SubresourceIndex = 0;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.pResource = stagingBuffer.Get();
		src.PlacedFootprint = placedTexture2D;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		command->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		this->transitionResource(command, i, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		command->Close();

		ID3D12CommandList* cmd[] = {
			command,
		};

		queue->ExecuteCommandLists(1, cmd);

		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		HANDLE fenceEvent = CreateEvent(0, FALSE, FALSE, 0);
		UINT64 fenceValue = 1;

		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.ReleaseAndGetAddressOf()));

		queue->Signal(fence.Get(), fenceValue);

		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);

		CloseHandle(fenceEvent);
	}

	stbi_image_free(pixels);

	return true;
}

void Texture::transitionResource(ID3D12GraphicsCommandList* command, UINT textureNum, D3D12_RESOURCE_BARRIER_FLAGS flag,
	D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = flag;
	barrier.Transition.pResource = m_resource[textureNum].Get();
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	command->ResourceBarrier(1, &barrier);
}