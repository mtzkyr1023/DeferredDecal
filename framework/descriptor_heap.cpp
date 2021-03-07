#include "descriptor_heap.h"

bool DescriptorHeap::create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, UINT descNum) {
	HRESULT res;

	D3D12_DESCRIPTOR_HEAP_DESC dhDesc{};
	dhDesc.NumDescriptors = descNum;
	dhDesc.Type = heapType;
	dhDesc.Flags = heapFlags;

	res = device->CreateDescriptorHeap(&dhDesc, IID_PPV_ARGS(m_descHeap.ReleaseAndGetAddressOf()));
	if (FAILED(res)) {
		MessageBox(NULL, "failed creating rtv heap.", "Error", MB_OK);
		return false;
	}

	m_descriptorSize = device->GetDescriptorHandleIncrementSize(heapType);

	return true;
}
