

#include "tools/input.h"
#include "tools/mesh_loader.h"


#include "imgui_dx12/imgui.h"
#include "imgui_dx12/imgui_impl_win32.h"
#include "imgui_dx12/imgui_impl_dx12.h"

#include "App.hpp"

App::App() {

}

App::~App() {

}

bool App::initialize(HWND hwnd) {
	UINT width = 1280;
	UINT height = 720;

	if (!m_device.create())
		return false;

	if (!m_queue.createGraphicsQueue(m_device.getDevice()))
		return false;

	if (!m_swapchain.create(m_queue.getQueue(), hwnd, backBufferCount, width, height, false))
		return false;

	if (!m_rtvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, backBufferCount))
		return false;

	if (!m_dsvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1))
		return false;

	m_commandAllocator.resize(backBufferCount);
	for (UINT i = 0; i < backBufferCount; i++) {
		if (!m_commandAllocator[i].createGraphicsCommandAllocator(m_device.getDevice()))
			return false;
	}

	m_commandList.resize(backBufferCount);
	for (UINT i = 0; i < backBufferCount; i++) {
		if (!m_commandList[i].createGraphicsCommandList(m_device.getDevice(), m_commandAllocator[i].getCommandAllocator()))
			return false;
	}

	if (!m_backBuffer.createBackBuffer(m_device.getDevice(), m_swapchain.getSwapchain(), backBufferCount))
		return false;

	if (!m_depthBuffer.createDepthStencilBuffer(m_device.getDevice(), 1, DXGI_FORMAT_R32_FLOAT,
		width, height))
		return false;

	if (!m_colorBuffer.createRenderTarget2D(m_device.getDevice(), 1, DXGI_FORMAT_R8G8B8A8_UNORM, m_width, m_height))
		return false;

	if (!m_normalBuffer.createRenderTarget2D(m_device.getDevice(), 1, DXGI_FORMAT_R16G16B16A16_FLOAT, m_width, m_height))
		return false;

	if (!m_cb0.create(m_device.getDevice(), backBufferCount * 2))
		return false;

	if (!m_cb2.create(m_device.getDevice(), backBufferCount))
		return false;

	if (!m_sb0.create(m_device.getDevice(), backBufferCount, decalCount, true))
		return false;

	if (!m_decalTexture.createResource(m_device.getDevice(), m_queue.getQueue(), 1, DXGI_FORMAT_R8G8B8A8_UNORM, "textures/blood.png", false))
		return false;

	m_geometoryRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1);
	m_geometoryRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	m_geometoryRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
	if (!m_geometoryRootSignature.create(m_device.getDevice(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS))
		return false;

	m_decalRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1);
	m_decalRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	m_decalRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
	m_decalRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
	if (!m_decalRootSignature.create(m_device.getDevice(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS))
		return false;

	m_lastRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	m_lastRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
	if (!m_lastRootSignature.create(m_device.getDevice(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS))
		return false;

	if (!m_geometoryVS.createVertexShader(L"shaders/test_vs.fx"))
		return false;

	if (!m_geometoryPS.createPixelShader(L"shaders/test_ps.fx"))
		return false;

	if (!m_decalVS.createVertexShader(L"shaders/deferred_decal_vs.fx"))
		return false;

	if (!m_decalVS.createVertexShader(L"shaders/deferred_decal_vs.fx"))
		return false;

	if (!m_decalPS.createPixelShader(L"shaders/deferred_decal_ps.fx"))
		return false;

	if (!m_screenVS.createVertexShader(L"shaders/screen_vs.fx"))
		return false;

	if (!m_lastPS.createPixelShader(L"shaders/last_ps.fx"))
		return false;

	m_geometoryPipeline.setVertexShader(m_geometoryVS.getByteCode());
	m_geometoryPipeline.setPixelShader(m_geometoryPS.getByteCode());
	m_geometoryPipeline.addInputLayout("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_geometoryPipeline.addInputLayout("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_geometoryPipeline.addInputLayout("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_geometoryPipeline.addInputLayout("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 0);
	m_geometoryPipeline.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_geometoryPipeline.addRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_geometoryPipeline.setDepthState(true, D3D12_COMPARISON_FUNC_LESS);
	m_geometoryPipeline.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_geometoryPipeline.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);

	m_decalPipeline.setVertexShader(m_decalVS.getByteCode());
	m_decalPipeline.setPixelShader(m_decalPS.getByteCode());
	m_decalPipeline.addInputLayout("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_decalPipeline.addInputLayout("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_decalPipeline.addInputLayout("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_decalPipeline.addInputLayout("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 0);
	m_decalPipeline.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_decalPipeline.setDepthState(false, D3D12_COMPARISON_FUNC_NEVER);
	m_decalPipeline.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_FRONT);

	m_lastPipeline.setVertexShader(m_screenVS.getByteCode());
	m_lastPipeline.setPixelShader(m_lastPS.getByteCode());
	m_lastPipeline.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_lastPipeline.setDepthState(false, D3D12_COMPARISON_FUNC_NEVER);
	m_lastPipeline.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);

	if (!m_geometoryPipeline.create(m_device.getDevice(), m_geometoryRootSignature.getRootSignature()))
		return false;

	if (!m_decalPipeline.create(m_device.getDevice(), m_decalRootSignature.getRootSignature()))
		return false;

	if (!m_lastPipeline.create(m_device.getDevice(), m_lastRootSignature.getRootSignature()))
		return false;

	if (!m_mesh.createFromGltf(m_device.getDevice(), m_queue.getQueue(), 1, "models/gltf/sponza.gltf"))
		return false;

	if (!m_cubeMesh.createFromGltf(m_device.getDevice(), m_queue.getQueue(), 1, "models/cube.gltf"))
		return false;

	if (!m_fence.create(m_device.getDevice()))
		return false;

	if (!m_geometoryCbvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		backBufferCount * 2 + m_mesh.getTextureCount()))
		return false;

	if (!m_geometorySamplerHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1))
		return false;

	if (!m_decalRtvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1))
		return false;
	if (!m_decalCbvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, backBufferCount * 2 + 2))
		return false;

	if (!m_decalSamplerHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1))
		return false;

	if (!m_lastSrvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1))
		return false;

	if (!m_lastSamplerHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1))
		return false;

	if (!m_geometoryRtvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 2))
		return false;

	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsDesc{};
		dsDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsDesc.Texture2D.MipSlice = 0;

		auto dsvHandle = m_dsvHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

		m_device.getDevice()->CreateDepthStencilView(m_depthBuffer.getResource(0), &dsDesc, dsvHandle);
	}

	{
		auto cbvHandle = m_geometoryCbvHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

		for (UINT i = 0; i < backBufferCount; i++) {
			D3D12_RENDER_TARGET_VIEW_DESC rtDesc{};
			rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtDesc.Texture2D.MipSlice = 0;
			rtDesc.Texture2D.PlaneSlice = 0;

			auto rtvHandle = m_rtvHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
			rtvHandle.ptr += i * m_rtvHeap.getDescriptorSize();

			m_device.getDevice()->CreateRenderTargetView(m_backBuffer.getResource(i), &rtDesc, rtvHandle);


			D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc{};
			cbDesc.BufferLocation = m_cb0.getResource(i * 2)->GetGPUVirtualAddress();
			cbDesc.SizeInBytes = sizeof(CB0);

			cbvHandle = m_geometoryCbvHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
			cbvHandle.ptr += i * m_geometoryCbvHeap.getDescriptorSize() * 2;

			m_device.getDevice()->CreateConstantBufferView(&cbDesc, cbvHandle);

			cbDesc.BufferLocation = m_cb0.getResource(i * 2 + 1)->GetGPUVirtualAddress();

			cbvHandle.ptr += m_geometoryCbvHeap.getDescriptorSize();

			m_device.getDevice()->CreateConstantBufferView(&cbDesc, cbvHandle);
		}

		for (UINT i = 0; i < m_mesh.getTextureCount(); i++) {
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0;

			cbvHandle.ptr += m_geometoryCbvHeap.getDescriptorSize();

			m_device.getDevice()->CreateShaderResourceView(m_mesh.getTexture(i)->getResource(0), &srvDesc, cbvHandle);
		}

		auto samplerHandle = m_geometorySamplerHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

		D3D12_SAMPLER_DESC samplerDesc{};
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.MaxLOD = FLT_MAX;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MipLODBias = 0.0f;

		m_device.getDevice()->CreateSampler(&samplerDesc, samplerHandle);
	}

	{
		auto rtvHandle = m_geometoryRtvHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
		D3D12_RENDER_TARGET_VIEW_DESC rtDesc{};
		rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtDesc.Texture2D.MipSlice = 0;
		rtDesc.Texture2D.PlaneSlice = 0;

		m_device.getDevice()->CreateRenderTargetView(m_colorBuffer.getResource(0), &rtDesc, rtvHandle);

		rtvHandle.ptr += m_geometoryRtvHeap.getDescriptorSize();

		rtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

		m_device.getDevice()->CreateRenderTargetView(m_normalBuffer.getResource(0), &rtDesc, rtvHandle);
	}

	{
		auto rtvHandle = m_decalRtvHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
		D3D12_RENDER_TARGET_VIEW_DESC rtDesc{};
		rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtDesc.Texture2D.MipSlice = 0;
		rtDesc.Texture2D.PlaneSlice = 0;

		m_device.getDevice()->CreateRenderTargetView(m_colorBuffer.getResource(0), &rtDesc, rtvHandle);
	}

	{
		auto cbvHandle = m_decalCbvHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

		for (UINT i = 0; i < backBufferCount; i++) {

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc{};
			cbDesc.BufferLocation = m_cb2.getResource(i)->GetGPUVirtualAddress();
			cbDesc.SizeInBytes = sizeof(CB2);

			m_device.getDevice()->CreateConstantBufferView(&cbDesc, cbvHandle);

			cbvHandle.ptr += m_decalCbvHeap.getDescriptorSize();

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			srvDesc.Buffer.NumElements = decalCount;
			srvDesc.Buffer.StructureByteStride = sizeof(SB0);
			
			m_device.getDevice()->CreateShaderResourceView(m_sb0.getResource(i), &srvDesc, cbvHandle);

			cbvHandle.ptr += m_decalCbvHeap.getDescriptorSize();
		}

		auto samplerHandle = m_decalSamplerHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping =
			D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0);
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0;

		m_device.getDevice()->CreateShaderResourceView(m_depthBuffer.getResource(0), &srvDesc, cbvHandle);

		cbvHandle.ptr += m_decalCbvHeap.getDescriptorSize();


		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		m_device.getDevice()->CreateShaderResourceView(m_decalTexture.getResource(0), &srvDesc, cbvHandle);

		D3D12_SAMPLER_DESC samplerDesc{};
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.MaxLOD = FLT_MAX;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MipLODBias = 0.0f;

		m_device.getDevice()->CreateSampler(&samplerDesc, samplerHandle);
	}


	{
		auto srvHandle = m_lastSrvHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0;

		m_device.getDevice()->CreateShaderResourceView(m_colorBuffer.getResource(0), &srvDesc, srvHandle);

		D3D12_SAMPLER_DESC samplerDesc{};
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.MaxLOD = FLT_MAX;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MipLODBias = 0.0f;

		auto samplerHandle = m_lastSamplerHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	
		m_device.getDevice()->CreateSampler(&samplerDesc, samplerHandle);
	}


	if (!m_gui.create(hwnd, m_device.getDevice(), DXGI_FORMAT_R8G8B8A8_UNORM, backBufferCount))
		return false;

	return true;
}

void App::shutdown() {
	m_queue.waitForFence(m_fence.getFence(), m_fence.getFenceEvent(), m_fence.getFenceValue());
	m_gui.destroy();
}

void App::render() {
	HRESULT res;
	UINT curImageCount = m_swapchain.getSwapchain()->GetCurrentBackBufferIndex();

	m_commandAllocator[curImageCount].getCommandAllocator()->Reset();
	ID3D12GraphicsCommandList* command = m_commandList[curImageCount].getCommandList();
	command->Reset(m_commandAllocator[curImageCount].getCommandAllocator(), nullptr);

	m_depthBuffer.transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	m_colorBuffer.transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_normalBuffer.transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto geometoryRtvHandle = m_geometoryRtvHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	auto dsvHandle = m_dsvHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

	command->OMSetRenderTargets(2, &geometoryRtvHandle, true, &dsvHandle);

	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	command->ClearRenderTargetView(geometoryRtvHandle, clearColor, 0, nullptr);
	geometoryRtvHandle.ptr += m_geometoryRtvHeap.getDescriptorSize();
	command->ClearRenderTargetView(geometoryRtvHandle, clearColor, 0, nullptr);
	command->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	D3D12_VIEWPORT viewport{};
	viewport.Width = 1280.0f;
	viewport.Height = 720.0f;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	D3D12_RECT scissor = { 0, 0, 1280, 720 };

	command->RSSetViewports(1, &viewport);
	command->RSSetScissorRects(1, &scissor);

	ID3D12DescriptorHeap* descHeaps[] = {
		m_geometoryCbvHeap.getDescriptorHeap(),
		m_geometorySamplerHeap.getDescriptorHeap(),
	};
	command->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

	command->SetGraphicsRootSignature(m_geometoryRootSignature.getRootSignature());

	auto cbvHandle = m_geometoryCbvHeap.getDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	auto samplerHandle = m_geometorySamplerHeap.getDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	cbvHandle.ptr += curImageCount * m_geometoryCbvHeap.getDescriptorSize() * 2;
	command->SetGraphicsRootDescriptorTable(0, cbvHandle);
	command->SetGraphicsRootDescriptorTable(2, samplerHandle);

	command->SetPipelineState(m_geometoryPipeline.getPipelineState());

	UINT vertexOffset = 0;
	UINT indexOffset = 0;
	UINT offset = 0;

	command->IASetVertexBuffers(0, 1, m_mesh.getVertexBuffer()->getVertexBuferView(0));
	command->IASetIndexBuffer(m_mesh.getIndexBuffer()->getIndexBufferView(0));
	command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	for (UINT i = 0; i < m_mesh.getMaterialCount(); i++) {
		cbvHandle = m_geometoryCbvHeap.getDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
		cbvHandle.ptr += m_geometoryCbvHeap.getDescriptorSize() * m_mesh.getAlbedoTextureINdex(i) +
			2 * m_geometoryCbvHeap.getDescriptorSize() * backBufferCount;
		command->SetGraphicsRootDescriptorTable(1, cbvHandle);

		command->DrawIndexedInstanced(m_mesh.getIndexCount(i), 1, indexOffset, vertexOffset, 0);

		vertexOffset += m_mesh.getVertexCount(i);
		indexOffset += m_mesh.getIndexCount(i);
	}

	cbvHandle = m_geometoryCbvHeap.getDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	cbvHandle.ptr += curImageCount * m_geometoryCbvHeap.getDescriptorSize() * 2;
	cbvHandle.ptr += m_geometoryCbvHeap.getDescriptorSize();
	command->SetGraphicsRootDescriptorTable(0, cbvHandle);

	command->IASetVertexBuffers(0, 1, m_cubeMesh.getVertexBuffer()->getVertexBuferView(0));
	command->IASetIndexBuffer(m_cubeMesh.getIndexBuffer()->getIndexBufferView(0));
	command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	command->DrawIndexedInstanced(m_cubeMesh.getAllIndexCount(), 1, 0, 0, 0);

	m_depthBuffer.transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	auto decalRtvHandle = m_decalRtvHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	command->OMSetRenderTargets(1, &decalRtvHandle, false, nullptr);
	
	command->SetPipelineState(m_decalPipeline.getPipelineState());

	command->SetGraphicsRootSignature(m_decalRootSignature.getRootSignature());

	ID3D12DescriptorHeap* decalHeaps[] = {
		m_decalCbvHeap.getDescriptorHeap(),
		m_decalSamplerHeap.getDescriptorHeap(),
	};
	command->SetDescriptorHeaps(_countof(decalHeaps), decalHeaps);

	auto decalCbvHandle = decalHeaps[0]->GetGPUDescriptorHandleForHeapStart();
	decalCbvHandle.ptr += curImageCount * m_decalCbvHeap.getDescriptorSize() * 2;
	command->SetGraphicsRootDescriptorTable(0, decalCbvHandle);
	decalCbvHandle.ptr += m_decalCbvHeap.getDescriptorSize();
	command->SetGraphicsRootDescriptorTable(1, decalCbvHandle);
	decalCbvHandle = decalHeaps[0]->GetGPUDescriptorHandleForHeapStart();
	decalCbvHandle.ptr += m_decalCbvHeap.getDescriptorSize() * 4;
	auto decalSamplerHandle = decalHeaps[1]->GetGPUDescriptorHandleForHeapStart();
	command->SetGraphicsRootDescriptorTable(2, decalCbvHandle);
	decalCbvHandle.ptr += m_decalCbvHeap.getDescriptorSize();
	command->SetGraphicsRootDescriptorTable(3, decalSamplerHandle);

	command->IASetVertexBuffers(0, 1, m_cubeMesh.getVertexBuffer()->getVertexBuferView(0));
	command->IASetIndexBuffer(m_cubeMesh.getIndexBuffer()->getIndexBufferView(0));
	command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	command->DrawIndexedInstanced(m_cubeMesh.getAllIndexCount(), m_decalCount, 0, 0, 0);

	m_colorBuffer.transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_normalBuffer.transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_backBuffer.transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto rtvHandle = m_rtvHeap.getDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += curImageCount * m_rtvHeap.getDescriptorSize();

	command->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	command->SetGraphicsRootSignature(m_lastRootSignature.getRootSignature());

	ID3D12DescriptorHeap* lastHeap[] = {
		m_lastSrvHeap.getDescriptorHeap(),
		m_lastSamplerHeap.getDescriptorHeap(),
	};

	command->SetDescriptorHeaps(2, lastHeap);

	auto lastSrvHandle = m_lastSrvHeap.getDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	auto lastSamplerHandle = m_lastSamplerHeap.getDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	command->SetGraphicsRootDescriptorTable(0, lastSrvHandle);
	command->SetGraphicsRootDescriptorTable(1, lastSamplerHandle);

	command->SetPipelineState(m_lastPipeline.getPipelineState());

	command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	command->DrawInstanced(4, 1, 0, 0);

	m_gui.renderFrame(command);

	m_backBuffer.transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	command->Close();

	ID3D12CommandList* cmdList[] = { command };
	m_queue.getQueue()->ExecuteCommandLists(_countof(cmdList), cmdList);


	res = m_device.getDevice()->GetDeviceRemovedReason();

	this->run((curImageCount + 1) % backBufferCount);

	m_queue.waitForFence(m_fence.getFence(), m_fence.getFenceEvent(), m_fence.getFenceValue());

	m_swapchain.getSwapchain()->Present(0, 0);
}


void App::run(UINT curImageCount) {
	
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("DeferredDecal");

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Text("WASDEQ:Move\nRightClick:Rotate View\nSPACE:Put Decal");

	ImGui::End();
	
	ImGui::Render();

	static glm::mat4 view, pose;
	static float rotateX, rotateY;
	static glm::vec3 pos;
	glm::quat quat;
	quat = glm::angleAxis(glm::radians(rotateY), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::angleAxis(glm::radians(rotateX), glm::vec3(1.0f, 0.0f, 0.0f));
	pose = glm::mat4(quat);
	glm::vec3 xVec = glm::vec3(pose[0][0], pose[0][1], pose[0][2]);
	glm::vec3 yVec = glm::vec3(pose[1][0], pose[1][1], pose[1][2]);
	glm::vec3 zVec = glm::vec3(pose[2][0], pose[2][1], pose[2][2]);

	if (Input::Instance().Push(DIK_W)) pos += zVec;
	if (Input::Instance().Push(DIK_S)) pos -= zVec;
	if (Input::Instance().Push(DIK_A)) pos -= xVec;
	if (Input::Instance().Push(DIK_D)) pos += xVec;
	if (Input::Instance().Push(DIK_E)) pos += yVec;
	if (Input::Instance().Push(DIK_Q)) pos -= yVec;

	rotateX += Input::Instance().GetMoveYRightPushed();
	rotateY -= Input::Instance().GetMoveXRightPushed();

	CB0 cb0;
	cb0.world = glm::transpose(glm::identity<glm::mat4>());
	cb0.view = glm::transpose(glm::lookAtLH(pos, pos + zVec, yVec));
	cb0.proj = glm::transpose(glm::perspectiveLH(glm::half_pi<float>(), 1280.0f / 720.0f, 0.1f, 1000.0f));

	m_cb0.updateBuffer(curImageCount * 2, sizeof(CB0), &cb0);

	cb0.world = glm::transpose(glm::translate(glm::identity<glm::mat4>(), pos + zVec * 10.0f));
	m_cb0.updateBuffer(curImageCount * 2 + 1, sizeof(CB0), &cb0);

	static bool isTrigger = false;
	if (isTrigger == true) {
		SB0 sb0;
		sb0.world = glm::translate(glm::identity<glm::mat4>(), pos + zVec * 10.0f) * glm::mat4(quat) * glm::scale(glm::identity<glm::mat4>(), glm::vec3(30.0f));
		sb0.invWorld = glm::transpose(glm::inverse(sb0.world));
		sb0.world = glm::transpose(sb0.world);

		SB0* ptr = m_sb0.getBuffer(curImageCount);
		memcpy_s(&ptr[m_decalID], sizeof(SB0), &sb0, sizeof(SB0));

		m_decalID = (++m_decalID) % decalCount;
		m_decalCount += m_decalCount != decalCount ? 1 : 0;

		isTrigger = false;
	}
	if (Input::Instance().Trigger(DIK_SPACE)) {
		SB0 sb0;
		sb0.world = glm::translate(glm::identity<glm::mat4>(), pos + zVec * 10.0f) * glm::mat4(quat) * glm::scale(glm::identity<glm::mat4>(), glm::vec3(30.0f));
		sb0.invWorld = glm::transpose(glm::inverse(sb0.world));
		sb0.world = glm::transpose(sb0.world);

		SB0* ptr = m_sb0.getBuffer(curImageCount);
		memcpy_s(&ptr[m_decalID], sizeof(SB0), &sb0, sizeof(SB0));

		isTrigger = true;
	}

	CB2 cb2;
	cb2.view = cb0.view;
	cb2.proj = cb0.proj;
	cb2.invViewProj = glm::transpose(glm::inverse(glm::perspectiveLH(glm::half_pi<float>(), 1280.0f / 720.0f, 0.1f, 1000.0f) *
		glm::lookAtLH(pos, pos + zVec, yVec)));
	cb2.screenParam = glm::vec4((float)m_width, (float)m_height, 0.0f, 0.0f);
	cb2.viewPos = glm::vec4(pos, 0.0f);

	m_cb2.updateBuffer(curImageCount, sizeof(CB2), &cb2);
}
