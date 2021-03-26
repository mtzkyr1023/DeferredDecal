
#include <random>
#include <utility>
#include <algorithm>

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

	if (!m_colorBuffer.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, DXGI_FORMAT_R8G8B8A8_UNORM, m_width, m_height))
		return false;

	if (!m_portalDepthBuffer.createDepthStencilBuffer(m_device.getDevice(), portalCount, DXGI_FORMAT_R32_FLOAT,
		width, height))
		return false;

	if (!m_portalColorBuffer.createRenderTarget2D(m_device.getDevice(), portalCount, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, DXGI_FORMAT_R8G8B8A8_UNORM, m_width, m_height))
		return false;

	m_geometoryRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1);
	m_geometoryRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	m_geometoryRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
	if (!m_geometoryRootSignature.create(m_device.getDevice(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS))
		return false;

	m_portalRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1);
	m_portalRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	m_portalRootSignature.addDeDescriptorCount(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
	if (!m_portalRootSignature.create(m_device.getDevice(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
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

	if (!m_portalVS.createVertexShader(L"shaders/portal_vs.fx"))
		return false;

	if (!m_portalPS.createPixelShader(L"shaders/portal_ps.fx"))
		return false;

	if (!m_screenVS.createVertexShader(L"shaders/screen_vs.fx"))
		return false;

	if (!m_lastPS.createPixelShader(L"shaders/last_ps.fx"))
		return false;

	m_geometoryPipeline.addInputLayout("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_geometoryPipeline.addInputLayout("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_geometoryPipeline.addInputLayout("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_geometoryPipeline.addInputLayout("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 0);
	m_geometoryPipeline.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_geometoryPipeline.setBlendState(BlendState::eLinear);
	m_geometoryPipeline.setDepthState(true, D3D12_COMPARISON_FUNC_LESS);
	m_geometoryPipeline.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_geometoryPipeline.setVertexShader(m_geometoryVS.getByteCode());
	m_geometoryPipeline.setPixelShader(m_geometoryPS.getByteCode());
	m_geometoryPipeline.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK);

	m_portalPipeline.addInputLayout("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_portalPipeline.addInputLayout("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_portalPipeline.addInputLayout("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 0);
	m_portalPipeline.addInputLayout("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 0);
	m_portalPipeline.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_portalPipeline.setBlendState(BlendState::eNone);
	m_portalPipeline.setDepthState(true, D3D12_COMPARISON_FUNC_LESS);
	m_portalPipeline.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_portalPipeline.setVertexShader(m_portalVS.getByteCode());
	m_portalPipeline.setPixelShader(m_portalPS.getByteCode());
	m_portalPipeline.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK);

	m_lastPipeline.setVertexShader(m_screenVS.getByteCode());
	m_lastPipeline.setPixelShader(m_lastPS.getByteCode());
	m_lastPipeline.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_lastPipeline.setDepthState(false, D3D12_COMPARISON_FUNC_NEVER);
	m_lastPipeline.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);

	if (!m_geometoryPipeline.create(m_device.getDevice(), m_geometoryRootSignature.getRootSignature()))
		return false;

	if (!m_portalPipeline.create(m_device.getDevice(), m_portalRootSignature.getRootSignature()))
		return false;

	if (!m_lastPipeline.create(m_device.getDevice(), m_lastRootSignature.getRootSignature()))
		return false;

	if (!m_mesh.createFromGltf(m_device.getDevice(), m_queue.getQueue(), 1, "models/gltf/sponza.gltf"))
		return false;

	if (!m_cubeMesh.createFromGltf(m_device.getDevice(), m_queue.getQueue(), 1, "models/cube.gltf"))
		return false;

	if (!m_cb0.create(m_device.getDevice(), backBufferCount))
		return false;

	if (!m_portalCB0.create(m_device.getDevice(), backBufferCount + portalCount + 1))
		return false;

	if (!m_camCB0.create(m_device.getDevice(), backBufferCount + portalCount))
		return false;

	if (!m_fence.create(m_device.getDevice()))
		return false;

	if (!m_geometoryCbvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		backBufferCount + m_mesh.getTextureCount()))
		return false;

	if (!m_geometorySamplerHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1))
		return false;

	if (!m_lastCbvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1))
		return false;

	if (!m_lastSamplerHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1))
		return false;

	if (!m_geometoryRtvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1))
		return false;

	if (!m_lastRtvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, backBufferCount))
		return false;

	if (!m_geometoryDsvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1))
		return false;

	if (!m_camCbvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		backBufferCount + portalCount + m_mesh.getTextureCount()))
		return false;

	if (!m_camSamplerHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1))
		return false;

	if (!m_camRtvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, portalCount))
		return false;

	if (!m_camDsvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, portalCount))
		return false;

	if (!m_portalCbvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		backBufferCount * portalCount + portalCount))
		return false;

	if (!m_portalSamplerHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1))
		return false;

	if (!m_portalRtvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1))
		return false;

	if (!m_portalDsvHeap.create(m_device.getDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1))
		return false;

	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsDesc{};
		dsDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsDesc.Texture2D.MipSlice = 0;

		auto dsvHandle = m_geometoryDsvHeap.getCpuHandle(0);

		m_device.getDevice()->CreateDepthStencilView(m_depthBuffer.getResource(0), &dsDesc, dsvHandle);
	}

	{

		for (UINT i = 0; i < backBufferCount; i++) {
			D3D12_RENDER_TARGET_VIEW_DESC rtDesc{};
			rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtDesc.Texture2D.MipSlice = 0;
			rtDesc.Texture2D.PlaneSlice = 0;

			auto rtvHandle = m_lastRtvHeap.getCpuHandle(i);

			m_device.getDevice()->CreateRenderTargetView(m_backBuffer.getResource(i), &rtDesc, rtvHandle);
		}
	}

	{

		for (UINT i = 0; i < backBufferCount; i++) {
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.BufferLocation = m_cb0.getResource(i)->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = sizeof(CB0);

			m_device.getDevice()->CreateConstantBufferView(&cbvDesc, m_geometoryCbvHeap.getCpuHandle(i));
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

			m_device.getDevice()->CreateShaderResourceView(m_mesh.getTexture(i)->getResource(0), &srvDesc,
				m_geometoryCbvHeap.getCpuHandle(backBufferCount + i));
		}

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

		m_device.getDevice()->CreateSampler(&samplerDesc, m_geometorySamplerHeap.getCpuHandle(0));
	}

	{
		D3D12_RENDER_TARGET_VIEW_DESC rtDesc{};
		rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtDesc.Texture2D.MipSlice = 0;
		rtDesc.Texture2D.PlaneSlice = 0;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;

		m_device.getDevice()->CreateRenderTargetView(m_colorBuffer.getResource(0), &rtDesc, m_geometoryRtvHeap.getCpuHandle(0));
		m_device.getDevice()->CreateDepthStencilView(m_depthBuffer.getResource(0), &dsvDesc, m_geometoryDsvHeap.getCpuHandle(0));
	}

	{
		for (UINT i = 0; i < backBufferCount; i++) {
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.BufferLocation = m_camCB0.getResource(i * 2)->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = sizeof(CB0);

			m_device.getDevice()->CreateConstantBufferView(&cbvDesc, m_camCbvHeap.getCpuHandle(i * 2));


			cbvDesc.BufferLocation = m_camCB0.getResource(i * 2 + 1)->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = sizeof(CB0);

			m_device.getDevice()->CreateConstantBufferView(&cbvDesc, m_camCbvHeap.getCpuHandle(i * 2 + 1));
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

			m_device.getDevice()->CreateShaderResourceView(m_mesh.getTexture(i)->getResource(0), &srvDesc,
				m_camCbvHeap.getCpuHandle(backBufferCount + portalCount + i));
		}

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

		m_device.getDevice()->CreateSampler(&samplerDesc, m_camSamplerHeap.getCpuHandle(0));
	}


	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0;

		m_device.getDevice()->CreateShaderResourceView(m_colorBuffer.getResource(0), &srvDesc, m_lastCbvHeap.getCpuHandle(0));

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
	
		m_device.getDevice()->CreateSampler(&samplerDesc, m_lastSamplerHeap.getCpuHandle(0));

		for (UINT i = 0; i < portalCount; i++) {
			D3D12_RENDER_TARGET_VIEW_DESC rtDesc{};
			rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtDesc.Texture2D.MipSlice = 0;
			rtDesc.Texture2D.PlaneSlice = 0;

			D3D12_DEPTH_STENCIL_VIEW_DESC dsDesc{};
			dsDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsDesc.Texture2D.MipSlice = 0;

			m_device.getDevice()->CreateRenderTargetView(m_portalColorBuffer.getResource(i), &rtDesc, m_camRtvHeap.getCpuHandle(i));

			m_device.getDevice()->CreateDepthStencilView(m_portalDepthBuffer.getResource(i), &dsDesc, m_camDsvHeap.getCpuHandle(i));
		}
	}

	{
		for (UINT i = 0; i < backBufferCount; i++) {
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.BufferLocation = m_portalCB0.getResource(i * 2)->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = sizeof(CB0);

			m_device.getDevice()->CreateConstantBufferView(&cbvDesc, m_portalCbvHeap.getCpuHandle(i * 2));

			cbvDesc.BufferLocation = m_portalCB0.getResource(i * 2 + 1)->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = sizeof(CB0);

			m_device.getDevice()->CreateConstantBufferView(&cbvDesc, m_portalCbvHeap.getCpuHandle(i * 2 + 1));
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0;

		m_device.getDevice()->CreateShaderResourceView(m_portalColorBuffer.getResource(0), &srvDesc, m_portalCbvHeap.getCpuHandle(backBufferCount + portalCount + 1));

		m_device.getDevice()->CreateShaderResourceView(m_portalColorBuffer.getResource(1), &srvDesc, m_portalCbvHeap.getCpuHandle(backBufferCount + portalCount));

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

		m_device.getDevice()->CreateSampler(&samplerDesc, m_portalSamplerHeap.getCpuHandle(0));

		D3D12_RENDER_TARGET_VIEW_DESC rtDesc{};
		rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtDesc.Texture2D.MipSlice = 0;
		rtDesc.Texture2D.PlaneSlice = 0;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;

		m_device.getDevice()->CreateRenderTargetView(m_colorBuffer.getResource(0), &rtDesc, m_portalRtvHeap.getCpuHandle(0));

		m_device.getDevice()->CreateDepthStencilView(m_depthBuffer.getResource(0), &dsvDesc, m_portalDsvHeap.getCpuHandle(0));
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

	{
		m_depthBuffer.transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		m_colorBuffer.transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		auto rtvHandle = m_geometoryRtvHeap.getCpuHandle(0);
		auto dsvHandle = m_geometoryDsvHeap.getCpuHandle(0);

		command->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);

		const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		command->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		command->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		ID3D12DescriptorHeap* descHeaps[] = {
			m_geometoryCbvHeap.getDescriptorHeap(),
			m_geometorySamplerHeap.getDescriptorHeap(),
		};
		command->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

		command->SetGraphicsRootSignature(m_geometoryRootSignature.getRootSignature());

		auto cbvHandle = m_geometoryCbvHeap.getGpuHandle(curImageCount);
		auto samplerHandle = m_geometorySamplerHeap.getGpuHandle(0);
		command->SetGraphicsRootDescriptorTable(0, cbvHandle);
		command->SetGraphicsRootDescriptorTable(2, samplerHandle);

		command->SetPipelineState(m_geometoryPipeline.getPipelineState());

		UINT vertexOffset = 0;
		UINT indexOffset = 0;

		command->IASetVertexBuffers(0, 1, m_mesh.getVertexBuffer()->getVertexBuferView(0));
		command->IASetIndexBuffer(m_mesh.getIndexBuffer()->getIndexBufferView(0));
		command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		for (UINT j = 0; j < m_mesh.getMaterialCount(); j++) {
			cbvHandle = m_geometoryCbvHeap.getGpuHandle(backBufferCount + m_mesh.getAlbedoTextureINdex(j));
			command->SetGraphicsRootDescriptorTable(1, cbvHandle);

			command->DrawIndexedInstanced(m_mesh.getIndexCount(j), 1, indexOffset, vertexOffset, 0);

			vertexOffset += m_mesh.getVertexCount(j);
			indexOffset += m_mesh.getIndexCount(j);
		}
	}

	for (UINT i = 0; i < portalCount; i++) {
		m_portalColorBuffer.transitionResource(command, i, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_portalDepthBuffer.transitionResource(command, i, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		auto rtvHandle = m_camRtvHeap.getCpuHandle(i);
		auto dsvHandle = m_camDsvHeap.getCpuHandle(i);

		command->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);

		const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		command->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		command->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		ID3D12DescriptorHeap* descHeaps[] = {
			m_camCbvHeap.getDescriptorHeap(),
			m_camSamplerHeap.getDescriptorHeap(),
		};
		command->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

		auto cbvHandle = m_camCbvHeap.getGpuHandle(curImageCount * backBufferCount + i);
		auto samplerHandle = m_camSamplerHeap.getGpuHandle(0);
		command->SetGraphicsRootDescriptorTable(0, cbvHandle);
		command->SetGraphicsRootDescriptorTable(2, samplerHandle);

		UINT vertexOffset = 0;
		UINT indexOffset = 0;

		command->IASetVertexBuffers(0, 1, m_mesh.getVertexBuffer()->getVertexBuferView(0));
		command->IASetIndexBuffer(m_mesh.getIndexBuffer()->getIndexBufferView(0));
		command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		for (UINT j = 0; j < m_mesh.getMaterialCount(); j++) {
			cbvHandle = m_camCbvHeap.getGpuHandle(backBufferCount + portalCount + m_mesh.getAlbedoTextureINdex(j));
			command->SetGraphicsRootDescriptorTable(1, cbvHandle);

			command->DrawIndexedInstanced(m_mesh.getIndexCount(j), 1, indexOffset, vertexOffset, 0);

			vertexOffset += m_mesh.getVertexCount(j);
			indexOffset += m_mesh.getIndexCount(j);
		}

		m_portalColorBuffer.transitionResource(command, i, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_portalDepthBuffer.transitionResource(command, i, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	{

		auto rtvHandle = m_geometoryRtvHeap.getCpuHandle(0);
		auto dsvHandle = m_geometoryDsvHeap.getCpuHandle(0);

		command->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		command->SetPipelineState(m_portalPipeline.getPipelineState());

		ID3D12DescriptorHeap* descHeaps[] = {
			m_portalCbvHeap.getDescriptorHeap(),
			m_portalSamplerHeap.getDescriptorHeap(),
		};

		command->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

		command->SetGraphicsRootSignature(m_portalRootSignature.getRootSignature());

		command->SetGraphicsRootDescriptorTable(2, m_portalSamplerHeap.getGpuHandle(0));

		command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		command->IASetVertexBuffers(0, 1, m_cubeMesh.getVertexBuffer()->getVertexBuferView(0));
		command->IASetIndexBuffer(m_cubeMesh.getIndexBuffer()->getIndexBufferView(0));

		for (UINT i = 0; i < portalCount; i++) {
			command->SetGraphicsRootDescriptorTable(0, m_portalCbvHeap.getGpuHandle(curImageCount * backBufferCount + i));
			command->SetGraphicsRootDescriptorTable(1, m_portalCbvHeap.getGpuHandle(backBufferCount + portalCount + i));

			command->DrawIndexedInstanced(m_cubeMesh.getAllIndexCount(), 1, 0, 0, 0);
		}

		m_depthBuffer.transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_colorBuffer.transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	m_backBuffer.transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	{
		auto rtvHandle = m_lastRtvHeap.getCpuHandle(curImageCount);
		command->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		command->SetGraphicsRootSignature(m_lastRootSignature.getRootSignature());

		ID3D12DescriptorHeap* lastHeap[] = {
			m_lastCbvHeap.getDescriptorHeap(),
			m_lastSamplerHeap.getDescriptorHeap(),
		};

		command->SetDescriptorHeaps(2, lastHeap);

		auto cbvHandle = m_lastCbvHeap.getGpuHandle(0);
		auto samplerHandle = m_lastSamplerHeap.getGpuHandle(0);
		command->SetGraphicsRootDescriptorTable(0, cbvHandle);
		command->SetGraphicsRootDescriptorTable(1, samplerHandle);

		command->SetPipelineState(m_lastPipeline.getPipelineState());

		command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		command->DrawInstanced(4, 1, 0, 0);

		m_gui.renderFrame(command);

		m_backBuffer.transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}

	command->Close();

	ID3D12CommandList* cmdList[] = { command };
	m_queue.getQueue()->ExecuteCommandLists(_countof(cmdList), cmdList);


	res = m_device.getDevice()->GetDeviceRemovedReason();

	this->run((curImageCount + 1) % backBufferCount);

	m_queue.waitForFence(m_fence.getFence(), m_fence.getFenceEvent(), m_fence.getFenceValue());

	m_swapchain.getSwapchain()->Present(0, 0);
}


void App::run(UINT curImageCount) {
	
	static float offset[3];
	static float cloudSize[3] = { 10.0f, 10.0f, 10.0f };
	static float cloudScale[3] = { 1.0f, 1.0f, 1.0f };
	static float cloudThreshold = 0.5f;
	static float portalPosition[2][3] = { -20.0f, 40.0f, 0.0f, 20.0f, 40.0f, 0.0f };
	static float portalScale[2][3] = { 20.0f, 40.0f, 2.0f, 20.0f, 40.0f, 2.0f };
	static float portalRotate[2][3] = {0.0f, 0.0f, 0.0f, 0.0f, -0.0f, 0.0f};

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Portal");

	if (ImGui::TreeNode("Portal1")) {
		ImGui::SliderFloat3("position", portalPosition[0], -1200.0f, 1200.0f);
		ImGui::SliderFloat3("scale", portalScale[0], -1.0f, 100.0f);
		ImGui::SliderFloat3("rotate", portalRotate[0], -360.0f, 0.0f);
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Portal2")) {
		ImGui::SliderFloat3("position", portalPosition[1], -1200.0f, 1200.0f);
		ImGui::SliderFloat3("scale", portalScale[1], -1.0f, 100.0f);
		ImGui::SliderFloat3("rotate", portalRotate[1], -360.0f, 0.0f);
		ImGui::TreePop();
	}

	ImGui::End();
	
	ImGui::Render();

	static glm::mat4 view, pose;
	static float rotateX, rotateY;
	static glm::vec3 pos;
	glm::quat quat;
	quat = glm::angleAxis(glm::radians(rotateY), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::angleAxis(glm::radians(rotateX), glm::vec3(1.0f, 0.0f, 0.0f));
	pose = glm::mat4(quat);
	glm::quat portalQuat1 = 
		glm::angleAxis(glm::radians(portalRotate[0][1]), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::angleAxis(glm::radians(portalRotate[0][0]), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat portalQuat2 = 
		glm::angleAxis(glm::radians(portalRotate[1][1]), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::angleAxis(glm::radians(portalRotate[1][0]), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 portalPose1 = glm::mat4(portalQuat1);
	glm::mat4 portalPose2 = glm::mat4(portalQuat2);
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

	glm::vec3 xportalVec1 = glm::vec3(portalPose1[0][0], portalPose1[0][1], portalPose1[0][2]);
	glm::vec3 yportalVec1 = glm::vec3(portalPose1[1][0], portalPose1[1][1], portalPose1[1][2]);
	glm::vec3 zportalVec1 = glm::vec3(portalPose1[2][0], portalPose1[2][1], portalPose1[2][2]);


	glm::vec3 xportalVec2 = glm::vec3(portalPose2[0][0], portalPose2[0][1], portalPose2[0][2]);
	glm::vec3 yportalVec2 = glm::vec3(portalPose2[1][0], portalPose2[1][1], portalPose2[1][2]);
	glm::vec3 zportalVec2 = glm::vec3(portalPose2[2][0], portalPose2[2][1], portalPose2[2][2]);

	glm::vec3 portalPos1 = glm::vec3(portalPosition[0][0], portalPosition[0][1], portalPosition[0][2]);
	glm::vec3 portalPos2 = glm::vec3(portalPosition[1][0], portalPosition[1][1], portalPosition[1][2]);

	float range1 = glm::length(pos - portalPos1);
	float range2 = glm::length(pos - portalPos2);

	glm::vec3 dir1 = portalPos1 - pos;
	glm::vec3 dir2 = portalPos2 - pos;
//	dir2.x = 1.0f - dir2.x;
	//dir1 *= glm::vec3(1.0f, -1.0f, 1.0f);
	//dir2 *= -1.0f;

	dir1 = -dir1 * glm::mat3(portalPose2);
	dir2 = -dir2 * glm::mat3(portalPose1);

	glm::vec3 mulzVec1 = glm::normalize(zVec * glm::mat3(portalPose1));
	glm::vec3 mulzVec2 = glm::normalize(zVec * glm::mat3(portalPose2));
	glm::vec3 mulyVec1 = glm::normalize(yVec * glm::mat3(portalPose1));
	glm::vec3 mulyVec2 = glm::normalize(yVec * glm::mat3(portalPose2));

	if (glm::dot(glm::normalize(dir1), zportalVec1) < 0.1f && glm::length(pos - portalPos1) < 20.0f) {
		pos = portalPos2;
		rotateY = portalRotate[1][1];
		rotateX = portalRotate[1][0];
	}
	else if (glm::dot(glm::normalize(dir2), zportalVec2) < 0.1f && glm::length(pos - portalPos2) < 20.0f) {
		pos = portalPos1;
		rotateY = portalRotate[0][1];
		rotateX = portalRotate[0][0];
	}

	CB0 cb0[5];
	cb0[0].world = glm::transpose(glm::identity<glm::mat4>());
	cb0[0].view = glm::transpose(glm::lookAtLH(pos, pos + zVec, yVec));
	cb0[0].proj = glm::transpose(glm::perspectiveLH(glm::half_pi<float>(), 1280.0f / 720.0f, 0.1f, 1000.0f));

	m_cb0.updateBuffer(curImageCount, sizeof(CB0), &cb0[0]);

	cb0[1].world = glm::transpose(glm::identity<glm::mat4>());
	cb0[1].view = glm::transpose(glm::lookAtLH(portalPos1 + dir2,
		portalPos1 + dir2 + mulzVec1, mulyVec1));
	cb0[1].proj = glm::transpose(glm::perspectiveLH(glm::half_pi<float>(), 1280.0f / 720.0f, 0.1f, 1000.0f));

	m_camCB0.updateBuffer(curImageCount * 2, sizeof(CB0), &cb0[1]);

	cb0[2].world = glm::transpose(glm::identity<glm::mat4>());
	cb0[2].view = glm::transpose(glm::lookAtLH(portalPos2 + dir1,
		portalPos2 + dir1 + mulzVec2, mulyVec2));
	cb0[2].proj = glm::transpose(glm::perspectiveLH(glm::half_pi<float>(), 1280.0f / 720.0f, 0.1f, 1000.0f));

	m_camCB0.updateBuffer(curImageCount * 2 + 1, sizeof(CB0), &cb0[2]);

	portalQuat1 =
		glm::angleAxis(glm::radians(portalRotate[0][1]), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::angleAxis(glm::radians(portalRotate[0][0]), glm::vec3(1.0f, 0.0f, 0.0f));
	portalQuat2 =
		glm::angleAxis(glm::radians(portalRotate[1][1]), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::angleAxis(glm::radians(portalRotate[1][0]), glm::vec3(1.0f, 0.0f, 0.0f));

	cb0[3].world = glm::transpose(glm::translate(glm::identity<glm::mat4>(), glm::vec3(portalPosition[0][0], portalPosition[0][1], portalPosition[0][2])) *
		portalPose1 * glm::scale(glm::identity<glm::mat4>(), glm::vec3(portalScale[0][0], portalScale[0][1], portalScale[0][2])));
	cb0[3].view = cb0[0].view;
	cb0[3].proj = cb0[0].proj;

	m_portalCB0.updateBuffer(curImageCount * 2, sizeof(CB0), &cb0[3]);

	cb0[4].world = glm::transpose(glm::translate(glm::identity<glm::mat4>(), glm::vec3(portalPosition[1][0], portalPosition[1][1], portalPosition[1][2])) *
		portalPose2 * glm::scale(glm::identity<glm::mat4>(), glm::vec3(portalScale[1][0], portalScale[1][1], portalScale[1][2])));
	cb0[4].view = cb0[0].view;
	cb0[4].proj = cb0[0].proj;

	m_portalCB0.updateBuffer(curImageCount * 2 + 1, sizeof(CB0), &cb0[4]);
}
