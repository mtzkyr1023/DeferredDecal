#ifndef _PIPELINE_H_
#define _PIPELINE_H_

#include <d3d12.h>
#include <dxcapi.h>

#include <wrl/client.h>
#include <vector>


enum class BlendState {
	eNone,
	eAdd,
	eLinear,
};

class Pipeline {
public:
	Pipeline() = default;
	~Pipeline() = default;

	bool create(ID3D12Device* device, ID3D12RootSignature* rootSignature);

	void addInputLayout(const char* name, DXGI_FORMAT format, UINT semanticIndex);
	void addRenderTargetFormat(DXGI_FORMAT format);
	void setDepthStencilFormat(DXGI_FORMAT format);
	void setDepthState(bool depthEnable, D3D12_COMPARISON_FUNC func);
	void setRasterState(D3D12_FILL_MODE fillMode, D3D12_CULL_MODE cullMode);
	void setBlendState(BlendState blendState);
	void setVertexShader(IDxcBlob* blob);
	void setPixelShader(IDxcBlob* blob);

	ID3D12PipelineState* getPipelineState() { return m_pipelineState.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_layout;
	D3D12_RASTERIZER_DESC m_rasterDesc;
	D3D12_COMPARISON_FUNC m_depthFunc;
	bool m_depthEnable;
	std::vector<DXGI_FORMAT> m_rtvFormat;
	DXGI_FORMAT m_depthStencilFormat;
	BlendState m_blendState = BlendState::eNone;
	IDxcBlob* m_vertexShader;
	IDxcBlob* m_pixelShader;
};


class ComputePipeline {
public:
	ComputePipeline() = default;
	~ComputePipeline() = default;

	bool create(ID3D12Device* device, ID3D12RootSignature* rootSignature);

	void setComputeShader(IDxcBlob* blob);

	ID3D12PipelineState* getPipelineState() { return m_pipelineState.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	IDxcBlob* m_computeShader;
};


#endif