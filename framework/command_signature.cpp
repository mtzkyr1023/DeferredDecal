#include "command_signature.h"


bool CommandSignature::create(ID3D12Device* device) {
	HRESULT res;

	D3D12_COMMAND_SIGNATURE_DESC csDesc{};

	D3D12_INDIRECT_ARGUMENT_DESC args[1];
	args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

	csDesc.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS);
	csDesc.NumArgumentDescs = ARRAYSIZE(args);
	csDesc.pArgumentDescs = args;
	csDesc.NodeMask = 1;

	res = device->CreateCommandSignature(&csDesc, nullptr, IID_PPV_ARGS(m_commandSignature.ReleaseAndGetAddressOf()));
	if (FAILED(res)) {
		return false;
	}

	return true;
}