#include "commandbuffer.h"


bool CommandAllocator::createGraphicsCommandAllocator(ID3D12Device* device) {
	HRESULT res;

	res = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf()));
	if (FAILED(res)) {
		MessageBox(NULL, "failed creating command allocator.", "Error", MB_OK);
		return false;
	}

	return true;
}



bool CommandList::createGraphicsCommandList(ID3D12Device* device, ID3D12CommandAllocator* commandPool) {
	HRESULT res;

	res = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandPool, nullptr,
		IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf()));
	if (FAILED(res)) {
		MessageBox(NULL, "failed creating command list.", "Error", MB_OK);
		return false;
	}

	m_commandList->Close();

	return true;
}