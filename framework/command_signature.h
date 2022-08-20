#ifndef _COMMAND_SIGNATURE_H_
#define _COMMAND_SIGNATURE_H_

#include <d3d12.h>
#include <wrl/client.h>


class CommandSignature {
public:
	CommandSignature() = default;
	~CommandSignature() = default;

	bool create(ID3D12Device* device);

	ID3D12CommandSignature* getCommandSignatue() { return m_commandSignature.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_commandSignature;
};

#endif