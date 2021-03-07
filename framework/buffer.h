#ifndef _BUFFER_H_
#define _BUFFER_H_


#include "resource.h"
#include "fence.h"


class VertexBuffer : public Resource {
public:
	VertexBuffer() = default;
	~VertexBuffer() = default;

	bool create(ID3D12Device* device, ID3D12CommandQueue* queue, UINT bufferCount, UINT stride, UINT size, void* data);

	D3D12_VERTEX_BUFFER_VIEW* getVertexBuferView(UINT num) { return &m_vertexBufferView[num]; }

private:
	std::vector<D3D12_VERTEX_BUFFER_VIEW> m_vertexBufferView;
};


class IndexBuffer : public Resource {
public:
	IndexBuffer() = default;
	~IndexBuffer() = default;

	bool create(ID3D12Device* device, ID3D12CommandQueue* queue, UINT bufferCount, UINT size, void* data);

	D3D12_INDEX_BUFFER_VIEW* getIndexBufferView(UINT num) { return &m_indexBufferView[num]; }

private:
	std::vector<D3D12_INDEX_BUFFER_VIEW> m_indexBufferView;
};

template<class T>
class ConstantBuffer : public Resource {
public:
	ConstantBuffer() = default;
	~ConstantBuffer() {
		for (auto& ite : m_resource)
			ite->Unmap(0, nullptr);
	}

	bool create(ID3D12Device* device, UINT bufferCount) {
		HRESULT res;

		m_resource.resize(bufferCount);
		m_constantBufferPtr.resize(bufferCount);

		for (UINT i = 0; i < bufferCount; i++) {
			D3D12_HEAP_PROPERTIES heapProp{};
			heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProp.CreationNodeMask = 1;
			heapProp.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC resDesc{};
			resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resDesc.Alignment = 0;
			resDesc.Width = sizeof(T);
			resDesc.Height = 1;
			resDesc.DepthOrArraySize = 1;
			resDesc.MipLevels = 1;
			resDesc.Format = DXGI_FORMAT_UNKNOWN;
			resDesc.SampleDesc.Count = 1;
			resDesc.SampleDesc.Quality = 0;
			resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			res = device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(m_resource[i].ReleaseAndGetAddressOf()));
			res = device->GetDeviceRemovedReason();
			if (FAILED(res))
				return false;

			m_resource[i]->Map(0, nullptr, reinterpret_cast<void**>(&m_constantBufferPtr[i]));
		}

		return true;
	}

	void updateBuffer(UINT bufferNum, UINT size, void* data) {
		memcpy_s(m_constantBufferPtr[bufferNum], size, data, size);
	}

	T* getBuffer(UINT num) { return m_constantBufferPtr[num]; }

private:
	std::vector<T*> m_constantBufferPtr;
};


template<class T>
class StructuredBuffer : public Resource {
public:
	StructuredBuffer() = default;
	~StructuredBuffer() {
		if (m_isCpuAccess) {
			for (auto& ite : m_resource)
				ite->Unmap(0, nullptr);
		}
	}

	bool create(ID3D12Device* device, UINT bufferCount, UINT elementCount, bool isCpuAccess) {
		HRESULT res;

		m_resource.resize(bufferCount);
		m_bufferPtr.resize(bufferCount);

		m_isCpuAccess = isCpuAccess;

		if (isCpuAccess) {
			for (UINT i = 0; i < bufferCount; i++) {
				D3D12_HEAP_PROPERTIES heapProp{};
				heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
				heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				heapProp.CreationNodeMask = 1;
				heapProp.VisibleNodeMask = 1;

				D3D12_RESOURCE_DESC resDesc{};
				resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				resDesc.Alignment = 0;
				resDesc.Width = sizeof(T) * elementCount;
				resDesc.Height = 1;
				resDesc.DepthOrArraySize = 1;
				resDesc.MipLevels = 1;
				resDesc.Format = DXGI_FORMAT_UNKNOWN;
				resDesc.SampleDesc.Count = 1;
				resDesc.SampleDesc.Quality = 0;
				resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

				res = device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr, IID_PPV_ARGS(m_resource[i].ReleaseAndGetAddressOf()));
				res = device->GetDeviceRemovedReason();
				if (FAILED(res))
					return false;

				m_resource[i]->Map(0, nullptr, reinterpret_cast<void**>(&m_bufferPtr[i]));
			}
		}

		return true;
	}

	void updateBuffer(UINT bufferNum, UINT size, void* data) {
		memcpy_s(m_bufferPtr[bufferNum], size, data, size);
	}

	T* getBuffer(UINT num) { return m_bufferPtr[num]; }

private:
	std::vector<T*> m_bufferPtr;
	bool m_isCpuAccess = false;
};

#endif