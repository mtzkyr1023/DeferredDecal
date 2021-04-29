#include "last_pass.h"

bool LastPass::create(ID3D12Device* dev, UINT backBufferCount, UINT width, UINT height) {
	
	m_width = width;
	m_height = height;
	m_rootSignature.addDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	m_rootSignature.addDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
	if (!m_rootSignature.create(dev,
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS))
		return false;

	if (!m_vs.createVertexShader(L"shaders/screen_vs.fx"))
		return false;
	if (!m_ps.createPixelShader(L"shaders/last_ps.fx"))
		return false;

	m_pipeline.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_pipeline.setBlendState(BlendState::eNone);
	m_pipeline.setDepthState(false, D3D12_COMPARISON_FUNC_NEVER);
	m_pipeline.setStencilEnable(false);
	m_pipeline.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);
	m_pipeline.setVertexShader(m_vs.getByteCode());
	m_pipeline.setPixelShader(m_ps.getByteCode());
	if (!m_pipeline.create(dev, m_rootSignature.getRootSignature()))
		return false;

	if (!m_rtvHeap.create(dev, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, backBufferCount))
		return false;
	
	if (!m_srvHeap.create(dev, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1))
		return false;

	if (!m_samplerHeap.create(dev, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1))
		return false;

	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		for (UINT i = 0; i < backBufferCount; i++) {
			auto rtvHandle = m_rtvHeap.getCpuHandle(i);
			ID3D12Resource* res = ResourceManager::Instance().getResource(BACK_BUFFER)->getResource(i);
			
			dev->CreateRenderTargetView(res, &rtvDesc, rtvHandle);
		}
	}

	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		auto srvHandle = m_srvHeap.getCpuHandle(0);
		ID3D12Resource* res = ResourceManager::Instance().getResource(ALBEDO_BUFFER)->getResource(0);

		dev->CreateShaderResourceView(res, &srvDesc, srvHandle);
	}

	{
		D3D12_SAMPLER_DESC samplerDesc{};
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.BorderColor[0] = 0.0f;
		samplerDesc.BorderColor[1] = 0.0f;
		samplerDesc.BorderColor[2] = 0.0f;
		samplerDesc.BorderColor[3] = 0.0f;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		samplerDesc.MaxAnisotropy = 16.0f;
		samplerDesc.MaxLOD = FLT_MAX;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MipLODBias = 0.0f;

		auto samplerHandle = m_samplerHeap.getCpuHandle(0);
		
		dev->CreateSampler(&samplerDesc, samplerHandle);
	}

	return true;
}

void LastPass::render(ID3D12GraphicsCommandList* command, UINT curImageCount) {
	D3D12_VIEWPORT viewport{};
	viewport.Width = (float)m_width;
	viewport.Height = (float)m_height;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	D3D12_RECT scissor = { 0, 0, m_width, m_height };

	command->RSSetViewports(1, &viewport);
	command->RSSetScissorRects(1, &scissor);

	auto rtvHandle = m_rtvHeap.getCpuHandle(curImageCount);
	float clearColor[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	command->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	command->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

	command->SetGraphicsRootSignature(m_rootSignature.getRootSignature());

	ID3D12DescriptorHeap* lastHeaps[] = {
		m_srvHeap.getDescriptorHeap(),
		m_samplerHeap.getDescriptorHeap(),
	};

	command->SetDescriptorHeaps(_countof(lastHeaps), lastHeaps);

	auto srvHandle = m_srvHeap.getGpuHandle(0);
	command->SetGraphicsRootDescriptorTable(0, srvHandle);

	auto samplerHandle = m_samplerHeap.getGpuHandle(0);
	command->SetGraphicsRootDescriptorTable(1, samplerHandle);

	command->SetPipelineState(m_pipeline.getPipelineState());

	command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	command->DrawInstanced(3, 1, 0, 0);

}

void LastPass::run(UINT curImageCount) {
	
}