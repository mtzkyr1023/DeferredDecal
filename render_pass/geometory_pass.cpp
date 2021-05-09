#include "geometory_pass.h"

#include "../tools/model.h"

bool GeometoryPass::create(ID3D12Device* device, ID3D12CommandQueue* queue, UINT backBufferCount, UINT width, UINT height) {
	m_width = width;
	m_height = height;

	m_rootSignature.addDescriptorCount(D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1);
	m_rootSignature.addDescriptorCount(D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	m_rootSignature.addDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	m_rootSignature.addDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
	if (!m_rootSignature.create(device,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS))
		return false;

	if (!m_vs.createVertexShader(L"shaders/simple_vs.fx"))
		return false;

	if (!m_ps.createPixelShader(L"shaders/simple_ps.fx"))
		return false;

	m_pipeline.addInputLayout("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_pipeline.addInputLayout("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_pipeline.addInputLayout("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_pipeline.addInputLayout("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 0);
	m_pipeline.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_pipeline.addRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_pipeline.addRenderTargetFormat(DXGI_FORMAT_R16G16_FLOAT);
	m_pipeline.setBlendState(BlendState::eNone);
	m_pipeline.setDepthState(true, D3D12_COMPARISON_FUNC_LESS_EQUAL);
	m_pipeline.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_pipeline.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK);
	m_pipeline.setStencilEnable(false);
	m_pipeline.setVertexShader(m_vs.getByteCode());
	m_pipeline.setPixelShader(m_ps.getByteCode());
	if (!m_pipeline.create(device, m_rootSignature.getRootSignature()))
		return false;

	if (!m_samplerHeap.create(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1))
		return false;

	if (!m_rtvHeap.create(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 3))
		return false;

	if (!m_dsvHeap.create(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1))
		return false;

	if (!m_mesh.createFromGltf(device, queue, 1, "models/cube.gltf"))
		return false;

	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		auto rtvHandle = m_rtvHeap.getCpuHandle(0);

		device->CreateRenderTargetView(ResourceManager::Instance().getResource(ALBEDO_BUFFER)->getResource(0), &rtvDesc, rtvHandle);

		rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

		rtvHandle = m_rtvHeap.getCpuHandle(1);

		device->CreateRenderTargetView(ResourceManager::Instance().getResource(NORMAL_BUFFER)->getResource(0), &rtvDesc, rtvHandle);

		rtvDesc.Format = DXGI_FORMAT_R16G16_FLOAT;

		rtvHandle = m_rtvHeap.getCpuHandle(2);

		device->CreateRenderTargetView(ResourceManager::Instance().getResource(PBR_BUFFER)->getResource(0), &rtvDesc, rtvHandle);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;

		auto dsvHandle = m_dsvHeap.getCpuHandle(0);

		device->CreateDepthStencilView(ResourceManager::Instance().getResource(DEPTH_BUFFER)->getResource(0), &dsvDesc, dsvHandle);
	}

	if (!m_cbvHeap.create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, backBufferCount * 2 + 1))
		return false;

	{
		auto cbvHandle = m_cbvHeap.getCpuHandle(0);
		ConstantBuffer* cb0 = ResourceManager::Instance().getResourceAsCB(VIEW_PROJ_BUFFER);
		for (uint32_t i = 0; i < backBufferCount; i++) {

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.SizeInBytes = sizeof(RenderPass::ViewProjBuffer);
			cbvDesc.BufferLocation = cb0->getResource(i)->GetGPUVirtualAddress();

			device->CreateConstantBufferView(&cbvDesc, cbvHandle);
			cbvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		device->CreateShaderResourceView(ResourceManager::Instance().getResource(SAMPLE_TEXTURE)->getResource(0), &srvDesc, cbvHandle);
	}

	return true;
}

void GeometoryPass::render(ID3D12GraphicsCommandList* command, UINT curImageCount) {
	auto rtvHandle = m_rtvHeap.getCpuHandle(0);
	auto dsvHandle = m_dsvHeap.getCpuHandle(0);

	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	command->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	command->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	command->OMSetRenderTargets(3, &rtvHandle, true, &dsvHandle);

	D3D12_VIEWPORT viewport{};
	viewport.Width = (float)m_width;
	viewport.Height = (float)m_height;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	D3D12_RECT scissor{};
	scissor.left = 0;
	scissor.top = 0;
	scissor.right = m_width;
	scissor.bottom = m_height;

	command->RSSetViewports(1, &viewport);
	command->RSSetScissorRects(1, &scissor);

	command->SetPipelineState(m_pipeline.getPipelineState());

	command->SetGraphicsRootSignature(m_rootSignature.getRootSignature());

	ID3D12DescriptorHeap* descHeaps[] = {
		m_cbvHeap.getDescriptorHeap(),
		m_samplerHeap.getDescriptorHeap(),
	};

	command->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

	auto cbvHandle = m_cbvHeap.getGpuHandle(curImageCount);
	auto srvHandle = m_cbvHeap.getGpuHandle(2 + m_simpleMeshRendererList.size() * 2);
	auto samplerHandle = m_samplerHeap.getGpuHandle(0);

	command->SetGraphicsRootDescriptorTable(0, cbvHandle);
	command->SetGraphicsRootDescriptorTable(2, srvHandle);
	command->SetGraphicsRootDescriptorTable(3, samplerHandle);

	command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	uint32_t i = 0;
	for (auto& ite : m_simpleMeshRendererList) {
		Mesh* mesh = ite->mesh;
		if (mesh == nullptr)
			mesh = &m_mesh;

		cbvHandle = m_cbvHeap.getGpuHandle(2 + m_simpleMeshRendererList.size() * 2 + i);
		command->SetGraphicsRootDescriptorTable(1, cbvHandle);

		command->IASetVertexBuffers(0, 1, mesh->getVertexBuffer()->getVertexBuferView(0));
		command->IASetIndexBuffer(mesh->getIndexBuffer()->getIndexBufferView(0));

		UINT vertexOffset = 0;
		UINT indexOffset = 0;
		for (UINT j = 0; j < mesh->getMaterialCount(); j++) {
			command->DrawIndexedInstanced(mesh->getIndexCount(j), 1, indexOffset, vertexOffset, 0);
			vertexOffset += mesh->getVertexCount(j);
			indexOffset += mesh->getIndexCount(j);
		}

		i++;
	}
}

void GeometoryPass::run(UINT curImageCount) {
	uint32_t i = 0;
	for (auto& ite : m_simpleMeshRendererList) {
		GameObject* obj = ite->parent;
		Transform* trans = obj->getComponent<Transform>();

		m_cb1Array[i].updateBuffer(curImageCount, sizeof(glm::mat4), &glm::transpose(trans->matrix));

		i++;
	}
}

bool GeometoryPass::setDescriptorHeap(ID3D12Device* device, UINT backBufferCount) {
	m_simpleMeshRendererList = Scheduler::instance().getComponentList<SimpleMeshRenderer>(LAYER::LAYER0);

	m_cb1Array.resize(m_simpleMeshRendererList.size());
	for (auto& ite : m_cb1Array) {
		if (!ite.create(device, sizeof(glm::mat4) * 4, backBufferCount))
			return false;
	}

	if (!m_cbvHeap.create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		backBufferCount + backBufferCount * m_simpleMeshRendererList.size() + 1))
		return false;

	{
		auto cbvHandle = m_cbvHeap.getCpuHandle(0);
		ConstantBuffer* cb0 = ResourceManager::Instance().getResourceAsCB(VIEW_PROJ_BUFFER);
		for (uint32_t i = 0; i < backBufferCount; i++) {

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.SizeInBytes = sizeof(RenderPass::ViewProjBuffer);
			cbvDesc.BufferLocation = cb0->getResource(i)->GetGPUVirtualAddress();

			device->CreateConstantBufferView(&cbvDesc, cbvHandle);
			cbvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		for (uint32_t i = 0; i < backBufferCount; i++) {
			for (uint32_t j = 0; j < m_simpleMeshRendererList.size(); j++) {
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
				cbvDesc.SizeInBytes = sizeof(glm::mat4) * 4;
				cbvDesc.BufferLocation = m_cb1Array[j].getResource(i)->GetGPUVirtualAddress();

				device->CreateConstantBufferView(&cbvDesc, cbvHandle);
				cbvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		device->CreateShaderResourceView(ResourceManager::Instance().getResource(SAMPLE_TEXTURE)->getResource(0), &srvDesc, cbvHandle);
	}

	return true;
}