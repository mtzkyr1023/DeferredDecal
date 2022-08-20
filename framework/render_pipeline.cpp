#include "render_pipeline.h"
#include "../defines.h"
#include "../tools/input.h"

#include <sstream>
#include <fstream>

bool RenderPipeline::initialize(HWND hwnd) {

	auto& resMgr = ResourceManager::Instance();

	m_device.create();

	m_queue.createGraphicsQueue(m_device.getDevice());

	m_swapchain.create(m_queue.getQueue(), hwnd, (UINT)kBackBufferCount, (UINT)kScreenWidth, (UINT)kScreenHeight, true);
	
	m_commandAllocator.resize((UINT)kBackBufferCount);
	for (UINT i = 0; i < (UINT)kBackBufferCount; i++) {
		if (!m_commandAllocator[i].createGraphicsCommandAllocator(m_device.getDevice()))
			return false;
	}

	int a = (UINT)kBackBufferCount;

	m_commandList.resize((UINT)a);

	for (UINT i = 0; i < (UINT)kBackBufferCount; i++) {
		if (!m_commandList[i].createGraphicsCommandList(m_device.getDevice(), m_commandAllocator[i].getCommandAllocator()))
			return false;
	}

	m_fence.create(m_device.getDevice());

	initializeResource();

	initializeResultPass();

	resMgr.updateDescriptorHeap(&m_device);

	m_camera = std::make_unique<Camera>();
	m_shadowcamera = std::make_unique<ShadowCamera>(glm::vec3(0.0f, 0.0f, 0.0f),
		glm::rotate(glm::identity<glm::quat>(), glm::degrees(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		512, 512, 1.0f, 1500.0f);
	m_cubecamera = std::make_unique<CubeCamera>(glm::vec3(0.0f, 0.0f, 0.0f));

	return true;
}


void RenderPipeline::destroy() {
	m_queue.waitForFence(m_fence.getFence(), m_fence.getFenceEvent(), m_fence.getFenceValue());
}


void RenderPipeline::render() {


	auto& resMgr = ResourceManager::Instance();

	if (Input::Instance().Trigger(DIK_SPACE)) {
		m_shadowcamera->transform().RotateWorld(m_camera->transform().angle().x, m_camera->transform().angle().y, 0.0f);
		m_shadowcamera->transform().position() = m_camera->transform().position();

	}
	UINT curImageCount = m_swapchain.getSwapchain()->GetCurrentBackBufferIndex();

	updateConstantBuffers((curImageCount + 1) % kBackBufferCount);

	m_commandAllocator[curImageCount].getCommandAllocator()->Reset();
	ID3D12GraphicsCommandList* command = m_commandList[curImageCount].getCommandList();
	command->Reset(m_commandAllocator[curImageCount].getCommandAllocator(), nullptr);
	
	{
		D3D12_VIEWPORT viewport{};
		viewport.Width = (float)kScreenWidth;
		viewport.Height = (float)kScreenHeight;
		viewport.MaxDepth = 1.0f;
		viewport.MinDepth = 0.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		D3D12_RECT scissor = { 0, 0, kScreenWidth, kScreenHeight };

		command->RSSetViewports(1, &viewport);
		command->RSSetScissorRects(1, &scissor);

		ID3D12DescriptorHeap* descHeaps[] = {
			resMgr.getShaderResourceHeap()->getDescriptorHeap(),
			resMgr.getSamplerHeap()->getDescriptorHeap(),
		};

		size_t heapCount = sizeof(descHeaps) / sizeof(descHeaps[0]);
		command->SetDescriptorHeaps(heapCount, descHeaps);
	}

	//renderVisibilityPass(command, curImageCount);


	writeResultBackBuffer(command, curImageCount);


	command->Close();

	ID3D12CommandList* cmdList[] = { command };
	m_queue.getQueue()->ExecuteCommandLists(_countof(cmdList), cmdList);


	m_queue.waitForFence(m_fence.getFence(), m_fence.getFenceEvent(), m_fence.getFenceValue());

	m_swapchain.getSwapchain()->Present(0, 0);
}

bool RenderPipeline::initializeResource() {

	auto& resMgr = ResourceManager::Instance();

	m_backbuffer = resMgr.createBackBuffer(m_device.getDevice(), m_swapchain.getSwapchain(), kBackBufferCount);

	m_depthBuffer = resMgr.createDepthStencilBuffer(m_device.getDevice(), kBackBufferCount, kScreenWidth, kScreenHeight, false);

	m_shadowMap = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		DXGI_FORMAT_R32G32_FLOAT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	m_reflectMap = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		DXGI_FORMAT_R8G8B8A8_UNORM, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	m_shadowDepthBuffer = resMgr.createDepthStencilBuffer(m_device.getDevice(), 1, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, false);

	m_fxaaTexture = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		DXGI_FORMAT_R8G8B8A8_UNORM, kScreenWidth, kScreenHeight);
	m_tempTexture = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		DXGI_FORMAT_R8G8B8A8_UNORM, kScreenWidth, kScreenHeight);

	m_testVs = resMgr.addVertexShader(L"shaders/test_vs.fx");
	m_testPs = resMgr.addPixelShader(L"shaders/test_ps.fx");
	m_simpleVs = resMgr.addVertexShader(L"shaders/simple_vs.fx");
	m_simplePs = resMgr.addPixelShader(L"shaders/simple_ps.fx");
	m_cubemapVs = resMgr.addVertexShader(L"shaders/cubemap_vs.fx");
	m_cubemapPs = resMgr.addPixelShader(L"shaders/cubemap_ps.fx");
	m_screenVs = resMgr.addVertexShader(L"shaders/screen_vs.fx");
	m_screenPs = resMgr.addPixelShader(L"shaders/last_ps.fx");

	m_shadowVs = resMgr.addVertexShader(L"shaders/shadow_vs.fx");
	m_shadowPs = resMgr.addPixelShader(L"shaders/shadow_ps.fx");

	m_fxaaCs = resMgr.addComputeShader(L"shaders/fxaa_cs.fx");

	{
		std::vector<std::string> filenames = {
			"textures/cubemap/02/posx.jpg",
			"textures/cubemap/02/negx.jpg",
			"textures/cubemap/02/posy.jpg",
			"textures/cubemap/02/negy.jpg",
			"textures/cubemap/02/posz.jpg",
			"textures/cubemap/02/negz.jpg",
		};
		m_cubeMap = resMgr.createCubeMap(m_device.getDevice(), m_queue.getQueue(), filenames, DXGI_FORMAT_R8G8B8A8_UNORM, false);
	}

	m_viewProjBuffer = resMgr.createConstantBuffer(m_device.getDevice(), (UINT)sizeof(glm::mat4) * 4, kBackBufferCount);
	m_shadowVPBuffer = resMgr.createConstantBuffer(m_device.getDevice(), (UINT)sizeof(glm::mat4) * 4, kBackBufferCount);

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

		m_wrapSampler = resMgr.addSamplerState(samplerDesc);

		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.BorderColor[0] = 1.0f;
		samplerDesc.BorderColor[1] = 1.0f;
		samplerDesc.BorderColor[2] = 1.0f;
		samplerDesc.BorderColor[3] = 1.0f;

		m_clampSampler = resMgr.addSamplerState(samplerDesc);
	}

	MeshManager::instance().createMeshFromGltf(m_device.getDevice(), m_queue.getQueue(), 1, "models/gltf/sponza.gltf");
	MeshManager::instance().createMeshFromGltf(m_device.getDevice(), m_queue.getQueue(), 1, "models/cube.gltf");
	MeshManager::instance().createMeshFromGltf(m_device.getDevice(), m_queue.getQueue(), 1, "models/sphere.gltf");

	m_screenInfoBuffer = resMgr.createConstantBuffer(m_device.getDevice(), sizeof(glm::mat4) * 4, kBackBufferCount);


	return true;
}


bool RenderPipeline::initializeResultPass() {

	auto& resMgr = ResourceManager::Instance();

	m_shadowMapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1
	);
	m_shadowMapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1
	);
	m_shadowMapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 1
	);
	m_shadowMapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		2, 1
	);
	m_shadowMapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		3, 1
	);
	m_shadowMapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		4, 1
	);
	m_shadowMapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		0, 1
	);

	m_shadowMapRootSignature.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);

	ShaderSp shadowVS = resMgr.GetShader(m_shadowVs);
	ShaderSp shadowPS = resMgr.GetShader(m_shadowPs);

	m_shadowMapPass.addRenderTargetFormat(DXGI_FORMAT_R32G32_FLOAT);
	m_shadowMapPass.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_shadowMapPass.setBlendState(BlendState::eNone);
	m_shadowMapPass.setDepthState(true, D3D12_COMPARISON_FUNC_LESS);
	m_shadowMapPass.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_shadowMapPass.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);
	m_shadowMapPass.setStencilEnable(false);
	m_shadowMapPass.setVertexShader(shadowVS->getByteCode());
	m_shadowMapPass.setPixelShader(shadowPS->getByteCode());

	if (!m_shadowMapPass.create(m_device.getDevice(), m_shadowMapRootSignature.getRootSignature()))
		return false;


	m_screenRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1);
	m_screenRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		0, 1);
	m_screenRootSignature.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);


	ShaderSp screenvs = resMgr.GetShader(m_screenVs);
	ShaderSp screenps = resMgr.GetShader(m_screenPs);

	m_screenPass.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_screenPass.setBlendState(BlendState::eNone);
	m_screenPass.setDepthState(false, D3D12_COMPARISON_FUNC_LESS);
	m_screenPass.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_screenPass.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);
	m_screenPass.setStencilEnable(false);
	m_screenPass.setVertexShader(screenvs->getByteCode());
	m_screenPass.setPixelShader(screenps->getByteCode());

	if (!m_screenPass.create(m_device.getDevice(), m_screenRootSignature.getRootSignature()))
		return false;



	m_fxaaRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1
	);
	m_fxaaRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1);
	m_fxaaRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		0, 1);
	m_fxaaRootSignature.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);

	ShaderSp fxaaCs = resMgr.GetShader(m_fxaaCs);
	m_fxaaPipeline.setComputeShader(fxaaCs->getByteCode());
	m_fxaaPipeline.create(m_device.getDevice(), m_fxaaRootSignature.getRootSignature());

	m_cubemapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1);
	m_cubemapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1);
	m_cubemapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 1);
	m_cubemapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		2, 1);
	m_cubemapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		0, 1);
	m_cubemapRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		3, 1
	);
	m_cubemapRootSignature.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);


	ShaderSp clearvs = resMgr.GetShader(m_cubemapVs);
	ShaderSp clearps = resMgr.GetShader(m_cubemapPs);

	m_cubemapPass.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_cubemapPass.setBlendState(BlendState::eNone);
	m_cubemapPass.setDepthState(true, D3D12_COMPARISON_FUNC_LESS_EQUAL);
	m_cubemapPass.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_cubemapPass.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);
	m_cubemapPass.setStencilEnable(false);
	m_cubemapPass.setVertexShader(clearvs->getByteCode());
	m_cubemapPass.setPixelShader(clearps->getByteCode());

	if (!m_cubemapPass.create(m_device.getDevice(), m_cubemapRootSignature.getRootSignature()))
		return false;

	m_simpleRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1);
	m_simpleRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1);
	m_simpleRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 1);
	m_simpleRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		2, 1);
	m_simpleRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		3, 1);
	m_simpleRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		0, 1);
	m_simpleRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		4, 1
	);
	m_simpleRootSignature.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);


	ShaderSp cubevs = resMgr.GetShader(m_simpleVs);
	ShaderSp cubeps = resMgr.GetShader(m_simplePs);

	m_simplePass.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_simplePass.setBlendState(BlendState::eNone);
	m_simplePass.setDepthState(true, D3D12_COMPARISON_FUNC_LESS);
	m_simplePass.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_simplePass.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);
	m_simplePass.setStencilEnable(false);
	m_simplePass.setVertexShader(cubevs->getByteCode());
	m_simplePass.setPixelShader(cubeps->getByteCode());

	if (!m_simplePass.create(m_device.getDevice(), m_simpleRootSignature.getRootSignature()))
		return false;


	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 1);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		2, 1);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		3, 1);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		4, 1);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		5, 1);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		6, 1);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		7, 1
	);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		8, 1
	);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		9, 1
	);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		0, 1);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		1, 1
	);
	m_writeResultRootSignature.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	ShaderSp vs = resMgr.GetShader(m_testVs);
	ShaderSp ps = resMgr.GetShader(m_testPs);

	m_writeResultPass.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_writeResultPass.addRenderTargetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_writeResultPass.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_writeResultPass.setBlendState(BlendState::eNone);
	m_writeResultPass.setDepthState(true, D3D12_COMPARISON_FUNC_LESS);
	m_writeResultPass.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_writeResultPass.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);
	m_writeResultPass.setStencilEnable(false);
	m_writeResultPass.setVertexShader(vs->getByteCode());
	m_writeResultPass.setPixelShader(ps->getByteCode());

	if (!m_writeResultPass.create(m_device.getDevice(), m_writeResultRootSignature.getRootSignature()))
		return false;

	return true;
}

void RenderPipeline::updateConstantBuffers(UINT curImageCount) {
	
	auto& resMgr = ResourceManager::Instance();
	const float scl = 0.25f;

	{
		m_camera->Update();
		ConstantBuffer* cb = resMgr.getResourceAsCB(m_viewProjBuffer);
		struct cb_t {
			glm::mat4 v;
			glm::mat4 p;
			glm::mat4 p1;
			glm::vec4 viewPos;
			glm::vec4 viewPos2;
			glm::vec4 padding[2];
		};
		cb_t source{};
		source.v = (m_camera->viewMatrix());
		source.p = (m_camera->projMatrix());
		source.p1 = glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f) * scl);
		source.viewPos = glm::vec4(m_camera->transform().position(), 1.0f);
		source.viewPos2 = glm::vec4(m_cubecamera->transform(0).position(), 0.0f);
		cb->updateBuffer(curImageCount, (UINT)sizeof(glm::mat4) * 4, &source);
	}
	{
		m_shadowcamera->Update();
		ConstantBuffer* cb = resMgr.getResourceAsCB(m_shadowVPBuffer);
		struct cb_t {
			glm::mat4 v;
			glm::mat4 p;
			glm::mat4 p1;
			glm::vec4 padding[4];
		};
		cb_t source{};
		source.v = (m_shadowcamera->viewMatrix());
		source.p = (m_shadowcamera->projMatrix());
		source.p1 = glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f) * scl);
		cb->updateBuffer(curImageCount, (UINT)sizeof(glm::mat4) * 4, &source);
	}
	{
		ConstantBuffer* cb = resMgr.getResourceAsCB(m_screenInfoBuffer);
		glm::vec2 screen((float)kScreenWidth, (float)kScreenHeight);
		cb->updateBuffer(curImageCount, sizeof(glm::vec2), &screen);
	}
}



void RenderPipeline::writeResultBackBuffer(ID3D12GraphicsCommandList* command, UINT curImageCount) {

	Model* sponzaModel = MeshManager::instance().getModel("models/gltf/sponza.gltf");
	//Model* cubeModel = MeshManager::instance().getModel("models/sphere.gltf");
	Model* cubeModel = MeshManager::instance().getModel("models/cube.gltf");

	auto& resMgr = ResourceManager::Instance();

	{
		Texture* shadowMap = static_cast<Texture*>(resMgr.getResource(m_shadowMap));
		Texture* reflectMap = static_cast<Texture*>(resMgr.getResource(m_reflectMap));
		Texture* depthBuffer = static_cast<Texture*>(resMgr.getResource(m_shadowDepthBuffer));

		shadowMap->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		reflectMap->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		depthBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		D3D12_VIEWPORT viewport{};
		viewport.Width = (float)SHADOW_MAP_SIZE;
		viewport.Height = (float)SHADOW_MAP_SIZE;
		viewport.MaxDepth = 1.0f;
		viewport.MinDepth = 0.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		D3D12_RECT scissor = { 0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE };

		command->RSSetViewports(1, &viewport);
		command->RSSetScissorRects(1, &scissor);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[] = {
			resMgr.getRenderTargetCpuHandle(m_shadowMap, 0),
			resMgr.getRenderTargetCpuHandle(m_reflectMap, 0),
		};

		auto dsvHandle = resMgr.getDepthStencilCpuHandle(m_shadowDepthBuffer, 0);
		command->OMSetRenderTargets(2, rtvHandle, false, &dsvHandle);
		float clearcolor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		command->ClearRenderTargetView(rtvHandle[0], clearcolor, 0, nullptr);
		command->ClearRenderTargetView(rtvHandle[1], clearcolor, 0, nullptr);
		command->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		command->SetPipelineState(m_shadowMapPass.getPipelineState());

		command->SetGraphicsRootSignature(m_shadowMapRootSignature.getRootSignature());

		{
			auto cbvHandle = resMgr.getShaderResourceGpuHandle(m_shadowVPBuffer, curImageCount);
			auto positionBufferSrv = resMgr.getShaderResourceGpuHandle(sponzaModel->getPositionBuffer(), 0);
			auto texcoordBufferSrv = resMgr.getShaderResourceGpuHandle(sponzaModel->getUVBuffer(), 0);
			auto indexBufferSrv = resMgr.getShaderResourceGpuHandle(sponzaModel->getIndexBuffer(), 0);
			auto indexOffsetHandle = resMgr.getShaderResourceGpuHandle(sponzaModel->getOffsetBuffer(), 0);
			auto samplerHandle = resMgr.getSamplerStateGpuHandle(m_wrapSampler, 0);

			command->SetGraphicsRootDescriptorTable(0, cbvHandle);
			command->SetGraphicsRootDescriptorTable(1, positionBufferSrv);
			command->SetGraphicsRootDescriptorTable(2, texcoordBufferSrv);
			command->SetGraphicsRootDescriptorTable(3, indexBufferSrv);
			command->SetGraphicsRootDescriptorTable(4, indexOffsetHandle);
			command->SetGraphicsRootDescriptorTable(6, samplerHandle);

			command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			int indexOffset = 0;
			for (int i = 0; i < sponzaModel->getMaterialCount(); i++) {
				auto albedoTexHandle = resMgr.getShaderResourceGpuHandle(sponzaModel->getAlbedoTexture(i), 0);
				command->SetGraphicsRootDescriptorTable(5, albedoTexHandle);

				command->DrawInstanced(sponzaModel->getIndexCount(i), i + 1, 0, i);

				indexOffset += sponzaModel->getIndexCount(i);
			}
		}

		shadowMap->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		reflectMap->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		depthBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	{
		Texture* backBuffer = static_cast<Texture*>(resMgr.getResource(m_backbuffer));
		Texture* fxaaBuffer = static_cast<Texture*>(resMgr.getResource(m_fxaaTexture));
		Texture* tempBuffer = static_cast<Texture*>(resMgr.getResource(m_tempTexture));
		Texture* depthBuffer = static_cast<Texture*>(resMgr.getResource(m_depthBuffer));

		fxaaBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		depthBuffer->transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		D3D12_VIEWPORT viewport{};
		viewport.Width = (float)kScreenWidth;
		viewport.Height = (float)kScreenHeight;
		viewport.MaxDepth = 1.0f;
		viewport.MinDepth = 0.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		D3D12_RECT scissor = { 0, 0, kScreenWidth, kScreenHeight };

		command->RSSetViewports(1, &viewport);
		command->RSSetScissorRects(1, &scissor);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[] = {
			resMgr.getRenderTargetCpuHandle(m_fxaaTexture, 0)
		};

		auto dsvHandle = resMgr.getDepthStencilCpuHandle(m_depthBuffer, curImageCount);
		command->OMSetRenderTargets(1, rtvHandle, false, &dsvHandle);
		float clearcolor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		command->ClearRenderTargetView(rtvHandle[0], clearcolor, 0, nullptr);
		command->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


		command->SetPipelineState(m_cubemapPass.getPipelineState());

		command->SetGraphicsRootSignature(m_cubemapRootSignature.getRootSignature());
		
		{
			auto cbvHandle = resMgr.getShaderResourceGpuHandle(m_viewProjBuffer, curImageCount);
			auto positionBufferSrv = resMgr.getShaderResourceGpuHandle(cubeModel->getPositionBuffer(), 0);
			auto indexBufferSrv = resMgr.getShaderResourceGpuHandle(cubeModel->getIndexBuffer(), 0);
			auto indexOffsetHandle = resMgr.getShaderResourceGpuHandle(cubeModel->getOffsetBuffer(), 0);
			auto samplerHandle = resMgr.getSamplerStateGpuHandle(m_wrapSampler, 0);
			auto cubeMapHandle = resMgr.getShaderResourceGpuHandle(m_cubeMap, 0);

			command->SetGraphicsRootDescriptorTable(0, cbvHandle);
			command->SetGraphicsRootDescriptorTable(1, positionBufferSrv);
			command->SetGraphicsRootDescriptorTable(2, indexBufferSrv);
			command->SetGraphicsRootDescriptorTable(3, indexOffsetHandle);
			command->SetGraphicsRootDescriptorTable(4, samplerHandle);
			command->SetGraphicsRootDescriptorTable(5, cubeMapHandle);

			command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			int indexOffset = 0;
			for (int i = 0; i < cubeModel->getMaterialCount(); i++) {

				command->DrawInstanced(cubeModel->getIndexCount(i), i + 1, 0, i);

				indexOffset += cubeModel->getIndexCount(i);
			}
		}

		command->SetPipelineState(m_writeResultPass.getPipelineState());

		command->SetGraphicsRootSignature(m_writeResultRootSignature.getRootSignature());


		{
			auto cbvHandle = resMgr.getShaderResourceGpuHandle(m_viewProjBuffer, curImageCount);
			auto positionBufferSrv = resMgr.getShaderResourceGpuHandle(sponzaModel->getPositionBuffer(), 0);
			auto normalBufferSrv = resMgr.getShaderResourceGpuHandle(sponzaModel->getNormalBuffer(), 0);
			auto tangentBufferSrv = resMgr.getShaderResourceGpuHandle(sponzaModel->getTangentBuffer(), 0);
			auto indexBufferSrv = resMgr.getShaderResourceGpuHandle(sponzaModel->getIndexBuffer(), 0);
			auto samplerHandle = resMgr.getSamplerStateGpuHandle(m_wrapSampler, 0);
			auto indexOffsetHandle = resMgr.getShaderResourceGpuHandle(sponzaModel->getOffsetBuffer(), 0);
			auto uvHandle = resMgr.getShaderResourceGpuHandle(sponzaModel->getUVBuffer(), 0);
			auto cubeMapHandle = resMgr.getShaderResourceGpuHandle(m_cubeMap, 0);
			auto clampsamplerHandle = resMgr.getSamplerStateGpuHandle(m_clampSampler, 0);

			command->SetGraphicsRootDescriptorTable(0, cbvHandle);
			command->SetGraphicsRootDescriptorTable(1, positionBufferSrv);
			command->SetGraphicsRootDescriptorTable(2, normalBufferSrv);
			command->SetGraphicsRootDescriptorTable(3, tangentBufferSrv);
			command->SetGraphicsRootDescriptorTable(4, uvHandle);
			command->SetGraphicsRootDescriptorTable(5, indexBufferSrv);
			command->SetGraphicsRootDescriptorTable(6, indexOffsetHandle);
			command->SetGraphicsRootDescriptorTable(10, cubeMapHandle);
			command->SetGraphicsRootDescriptorTable(11, samplerHandle);
			command->SetGraphicsRootDescriptorTable(12, clampsamplerHandle);

			command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			int indexOffset = 0;
			for (int i = 0; i < sponzaModel->getMaterialCount(); i++) {
				auto albedoSrvHandle = resMgr.getShaderResourceGpuHandle(sponzaModel->getAlbedoTexture(i), 0);
				auto normalSrvHandle = resMgr.getShaderResourceGpuHandle(sponzaModel->getNormalTexture(i), 0);
				auto roughMetalSrvHandle = resMgr.getShaderResourceGpuHandle(sponzaModel->getRoughMetalTexture(i), 0);
				command->SetGraphicsRootDescriptorTable(7, albedoSrvHandle);
				command->SetGraphicsRootDescriptorTable(8, normalSrvHandle);
				command->SetGraphicsRootDescriptorTable(9, roughMetalSrvHandle);

				command->DrawInstanced(sponzaModel->getIndexCount(i), i + 1, 0, i);

				indexOffset += sponzaModel->getIndexCount(i);
			}
			
		}

		fxaaBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		tempBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		depthBuffer->transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		{
			auto cbvHandle = resMgr.getShaderResourceGpuHandle(m_screenInfoBuffer, curImageCount);
			auto sourceTexHandle = resMgr.getShaderResourceGpuHandle(m_fxaaTexture, 0);
			auto destTexHandle = resMgr.getShaderResourceGpuHandle(m_tempTexture, 0);
			command->SetPipelineState(m_fxaaPipeline.getPipelineState());

			command->SetComputeRootSignature(m_fxaaRootSignature.getRootSignature());

			command->SetComputeRootDescriptorTable(0, cbvHandle);
			command->SetComputeRootDescriptorTable(1, sourceTexHandle);
			command->SetComputeRootDescriptorTable(2, destTexHandle);

			command->Dispatch(kScreenWidth / 16, kScreenHeight / 16, 1);
		}


		tempBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		backBuffer->transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		{
			auto sourceTexHandle = resMgr.getShaderResourceGpuHandle(m_tempTexture, 0);
			auto samplerHandle = resMgr.getSamplerStateGpuHandle(m_wrapSampler, 0);

			rtvHandle[0] = resMgr.getRenderTargetCpuHandle(m_backbuffer, curImageCount);
			command->OMSetRenderTargets(1, rtvHandle, false, &dsvHandle);

			command->SetPipelineState(m_screenPass.getPipelineState());
			command->SetGraphicsRootSignature(m_screenRootSignature.getRootSignature());

			command->SetGraphicsRootDescriptorTable(0, sourceTexHandle);
			command->SetGraphicsRootDescriptorTable(1, samplerHandle);

			command->DrawInstanced(4, 1, 0, 0);
		}

		backBuffer->transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}
}