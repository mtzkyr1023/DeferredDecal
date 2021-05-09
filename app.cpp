
#include <random>
#include <utility>
#include <algorithm>

#include "tools/input.h"
#include "tools/mesh_loader.h"

#include "system/oct_tree.h"

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

	if (!ResourceManager::Instance().createBackBuffer(BACK_BUFFER, m_device.getDevice(), m_swapchain.getSwapchain(), backBufferCount))
		return false;

	if (!ResourceManager::Instance().createRenderTarget2D(ALBEDO_BUFFER, m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, DXGI_FORMAT_R8G8B8A8_UNORM, width, height))
		return false;

	if (!ResourceManager::Instance().createRenderTarget2D(NORMAL_BUFFER, m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, DXGI_FORMAT_R16G16B16A16_FLOAT, width, height))
		return false;

	if (!ResourceManager::Instance().createRenderTarget2D(PBR_BUFFER, m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, DXGI_FORMAT_R16G16_FLOAT, width, height))
		return false;

	if (!ResourceManager::Instance().createDepthStencilBuffer(DEPTH_BUFFER, m_device.getDevice(), 1, m_width, m_height, false))
		return false;

	if (!ResourceManager::Instance().createTexture(SAMPLE_TEXTURE, m_device.getDevice(), m_queue.getQueue(), 1, DXGI_FORMAT_R8G8B8A8_UNORM, "textures/default_color.bmp", false))
		return false;

	if (!ResourceManager::Instance().createConstantBuffer(VIEW_PROJ_BUFFER, m_device.getDevice(), sizeof(RenderPass::ViewProjBuffer), backBufferCount))
		return false;


	if (!m_lastPass.create(m_device.getDevice(), backBufferCount, width, height))
		return false;

	if (!m_geoPass.create(m_device.getDevice(), m_queue.getQueue(), backBufferCount, m_width, m_height))
		return false;

	if (!m_gui.create(hwnd, m_device.getDevice(), DXGI_FORMAT_R8G8B8A8_UNORM, backBufferCount))
		return false;

	if (!m_fence.create(m_device.getDevice()))
		return false;

	m_scene.init();

	if (!m_geoPass.setDescriptorHeap(m_device.getDevice(), backBufferCount))
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

	{
		Texture* albedoBuffer = static_cast<Texture*>(ResourceManager::Instance().getResource(ALBEDO_BUFFER));
		Texture* normalBuffer = static_cast<Texture*>(ResourceManager::Instance().getResource(NORMAL_BUFFER));
		Texture* pbrBuffer = static_cast<Texture*>(ResourceManager::Instance().getResource(PBR_BUFFER));
		Texture* depthBuffer = static_cast<Texture*>(ResourceManager::Instance().getResource(DEPTH_BUFFER));

		albedoBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		normalBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pbrBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		depthBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		m_geoPass.render(command, curImageCount);

		albedoBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		normalBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pbrBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		depthBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	{
		Texture* backBuffer = static_cast<Texture*>(ResourceManager::Instance().getResource(BACK_BUFFER));
		
		backBuffer->transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		
		m_lastPass.render(command, curImageCount);

		m_gui.renderFrame(command);

		backBuffer->transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}

	command->Close();

	ID3D12CommandList* cmdList[] = { command };
	m_queue.getQueue()->ExecuteCommandLists(_countof(cmdList), cmdList);


	this->run((curImageCount + 1) % backBufferCount);

	m_queue.waitForFence(m_fence.getFence(), m_fence.getFenceEvent(), m_fence.getFenceValue());

	m_swapchain.getSwapchain()->Present(0, 0);
}


void App::run(UINT curImageCount) {
	

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("");

	ImGui::End();
	
	ImGui::Render();

	m_scene.run(curImageCount, 1.0f);

	Scheduler::instance().execute(1.0f);

	m_geoPass.run(curImageCount);
	m_lastPass.run(curImageCount);
}
