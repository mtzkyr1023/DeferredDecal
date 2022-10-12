#include "render_pipeline.h"
#include "../defines.h"
#include "../tools/input.h"
#include "../imgui_dx12/imgui.h"
#include "../imgui_dx12/imgui_impl_win32.h"
#include "../imgui_dx12/imgui_impl_dx12.h"

#include <sstream>
#include <fstream>
#include <thread>

bool RenderPipeline::initialize(HWND hwnd) {

	auto& resMgr = ResourceManager::Instance();

	m_device.create();

	m_queue.createGraphicsQueue(m_device.getDevice());

	m_swapchain.create(m_queue.getQueue(), hwnd, (UINT)kBackBufferCount, (UINT)kScreenWidth, (UINT)kScreenHeight, false);
	
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


	m_camera2 = std::make_unique<Camera>();
	m_shadowcamera = std::make_unique<ShadowCamera>(glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(90.0f, 0.0f, 0.0f),
		512, 512, 1.0f, 800.0f);
	m_cubecamera = std::make_unique<CubeCamera>(glm::vec3(0.0f, 0.0f, 0.0f));

	m_gui.create(hwnd, m_device.getDevice(), DXGI_FORMAT_R8G8B8A8_UNORM, kBackBufferCount, ResourceManager::Instance().getShaderResourceHeap()->getDescriptorHeap());

	
	//m_camera->transform().setParent(&m_player.transform());

	return true;
}


void RenderPipeline::destroy() {
	m_queue.waitForFence(m_fence.getFence(), m_fence.getFenceEvent(), m_fence.getFenceValue());
	m_gui.destroy();
}


void RenderPipeline::render() {

	CollisionManager::instance().simulate(ImGui::GetIO().DeltaTime);

	m_collisionDebugDrawer.clearLines();

	CollisionManager::instance().getWorld()->debugDrawWorld();


	auto& resMgr = ResourceManager::Instance();

	m_player.update(ImGui::GetIO().DeltaTime);

	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();


		ImGui::Text("deltaTime: %.4f", ImGui::GetIO().DeltaTime);
		ImGui::Text("framerate: %.2f", ImGui::GetIO().Framerate);

		ImGui::Text("x:%fy:%fz:%f", m_player.transform().position().x, m_player.transform().position().y, m_player.transform().position().z);

		{
			D3D12_GPU_DESCRIPTOR_HANDLE sourceTexHandle = resMgr.getShaderResourceGpuHandle(m_reflectMap, 0);
			ImGui::Image((sourceTexHandle.ptr), ImVec2(256.0f, 256.0f));
		}

		ImGui::Render();
	}


	m_shadowcamera->transform().AddRotation(0.0f, (float)Input::Instance().GetMoveXLeftPushed() * 0.01f, (float)Input::Instance().GetMoveYLeftPushed() * 0.01f);

	UINT curImageCount = m_swapchain.getSwapchain()->GetCurrentBackBufferIndex();

	updateConstantBuffers((curImageCount + 1) % kBackBufferCount);

	std::thread th1([this]() {
		auto& resMgr = ResourceManager::Instance();
		UINT curImageCount = (m_swapchain.getSwapchain()->GetCurrentBackBufferIndex() + 1) % kBackBufferCount;
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


		writeResultBackBuffer(command, curImageCount);


		command->Close();
	});

	ID3D12GraphicsCommandList* command = m_commandList[curImageCount].getCommandList();

	ID3D12CommandList* cmdList[] = { command };
	m_queue.getQueue()->ExecuteCommandLists(_countof(cmdList), cmdList);


	m_queue.waitForFence(m_fence.getFence(), m_fence.getFenceEvent(), m_fence.getFenceValue());

	m_swapchain.getSwapchain()->Present(0, 0);

	th1.join();
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

	m_shadowBlurXBuffer = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		DXGI_FORMAT_R32G32_FLOAT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	m_shadowBlurYBuffer = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		DXGI_FORMAT_R32G32_FLOAT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

	m_albedoTexture = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		DXGI_FORMAT_R8G8B8A8_UNORM, kScreenWidth, kScreenHeight);
	m_normalTexture = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		DXGI_FORMAT_R32G32B32A32_FLOAT, kScreenWidth, kScreenHeight);
	m_roughMetalTexture = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		DXGI_FORMAT_R32G32B32A32_FLOAT, kScreenWidth, kScreenHeight);

	m_fxaaTexture = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		DXGI_FORMAT_R8G8B8A8_UNORM, kScreenWidth, kScreenHeight);
	m_tempTexture = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		DXGI_FORMAT_R8G8B8A8_UNORM, kScreenWidth, kScreenHeight);

	{
		struct PointLight {
			glm::vec3 pos;
			float radius;
			glm::vec3 color;
			float atten;
		};

		std::vector<PointLight> pointLight;

		for (int i = 0; i < 16; i++) {
			for (int j = 0; j < 16; j++) {
				PointLight elem{};
				elem.pos = glm::vec3((float)i * 16.0f - 128.0f, 0.0f, (float)j * 16.0f - 128.0f);
				elem.color = glm::vec3((float)i / 16.0f, 1.0f, (float)j / 16.0f);
				elem.radius = 8.0f;
				elem.atten = 16.0f;
				pointLight.push_back(elem);
			}
		}

		m_pointLightBuffer = resMgr.createStructuredBuffer(m_device.getDevice(), m_queue.getQueue(), 1, sizeof(PointLight), pointLight.size(), pointLight.data());

		m_lightIndexOffsetBuffer = resMgr.createStructuredBuffer(m_device.getDevice(), 1, sizeof(int), FRUSTUM_SIZE_X * FRUSTUM_SIZE_Y * FRUSTUM_SIZE_Z, false, true);
		m_lightIndexBuffer = resMgr.createStructuredBuffer(m_device.getDevice(), 1, sizeof(int), FRUSTUM_SIZE_X * FRUSTUM_SIZE_Y * FRUSTUM_SIZE_Z * 256, false, true);
	}
	{
		m_lineBuffer = resMgr.createStructuredBuffer(m_device.getDevice(), kBackBufferCount, sizeof(CollisionDebugDraw::LineInfo), 2048, true, false);
	}
	{
		struct Frustum {
			glm::vec4 Min;
			glm::vec4 Max;
		};

		std::vector<Frustum> frustum(Camera::FRUSTUM_DIV_SIZE_X * Camera::FRUSTUM_DIV_SIZE_Y * Camera::FRUSTUM_DIV_SIZE_Z);
		for (int z = 0; z < Camera::FRUSTUM_DIV_SIZE_Z; z++) {
			for (int y = 0; y < Camera::FRUSTUM_DIV_SIZE_Y; y++) {
				for (int x = 0; x < Camera::FRUSTUM_DIV_SIZE_X; x++) {
					int cellIndex = z * Camera::FRUSTUM_DIV_SIZE_Z * Camera::FRUSTUM_DIV_SIZE_Y + y * Camera::FRUSTUM_DIV_SIZE_Y + x;
					frustum[cellIndex].Min = m_player.camera()->frustumMin()[cellIndex];
					frustum[cellIndex].Max = m_player.camera()->frustumMax()[cellIndex];
				}
			}
		}

		m_frustumBuffer = resMgr.createStructuredBuffer(m_device.getDevice(), m_queue.getQueue(), 1, sizeof(Frustum), frustum.size(), frustum.data());
	}
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

	m_blurXCs = resMgr.addComputeShader(L"shaders/blur_x_cs.fx");
	m_blurYCs = resMgr.addComputeShader(L"shaders/blur_y_cs.fx");

	m_deferredPs = resMgr.addPixelShader(L"shaders/deferred_ps.fx");

	m_lineVs = resMgr.addVertexShader(L"shaders/line_vs.fx");
	m_linePs = resMgr.addPixelShader(L"shaders/line_ps.fx");

	m_fxaaCs = resMgr.addComputeShader(L"shaders/fxaa_cs.fx");

	m_lightCullCs = resMgr.addComputeShader(L"shaders/clustered_cull_cs.fx");

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
	m_shadowVPBuffer = resMgr.createConstantBuffer(m_device.getDevice(), (UINT)sizeof(glm::mat4) * 8, kBackBufferCount);
	m_blurInfoBuffer = resMgr.createConstantBuffer(m_device.getDevice(), (UINT)sizeof(glm::mat4) * 4, kBackBufferCount);
	m_transposeVPBuffer = resMgr.createConstantBuffer(m_device.getDevice(), (UINT)sizeof(glm::mat4) * 4, kBackBufferCount);

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
		samplerDesc.BorderColor[0] = 0.0f;
		samplerDesc.BorderColor[1] = 0.0f;
		samplerDesc.BorderColor[2] = 0.0f;
		samplerDesc.BorderColor[3] = 0.0f;

		m_clampSampler = resMgr.addSamplerState(samplerDesc);
	}

	MeshManager::instance().createMeshFromGltf(m_device.getDevice(), m_queue.getQueue(), 1, "models/gltf/sponza.gltf");
	MeshManager::instance().createMeshFromGltf(m_device.getDevice(), m_queue.getQueue(), 1, "models/cube.gltf");
	MeshManager::instance().createMeshFromGltf(m_device.getDevice(), m_queue.getQueue(), 1, "models/sphere.gltf");
	MeshManager::instance().createMeshFromGltf(m_device.getDevice(), m_queue.getQueue(), 1, "models/unitychan.gltf");
	MeshManager::instance().createMeshFromGltf(m_device.getDevice(), m_queue.getQueue(), 1, "models/Duck/gltf/Duck.gltf");

	m_screenInfoBuffer = resMgr.createConstantBuffer(m_device.getDevice(), sizeof(glm::mat4) * 4, kBackBufferCount);

	{
		auto dynamicsWorld = CollisionManager::instance().getWorld();

		dynamicsWorld->setGravity(btVector3(0, -10, 0));

		///create a few basic rigid bodies
		btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(100.), btScalar(5.), btScalar(100.)));

		//keep track of the shapes, we release memory at exit.
		//make sure to re-use collision shapes among rigid bodies whenever possible!
		btAlignedObjectArray<btCollisionShape*> collisionShapes;

		collisionShapes.push_back(groundShape);

		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, -0, 0));

		{
			m_groundRigidBody.reset(new CollisionRigidBody(glm::vec3(100.0f, 5.0f, 100.0f), 0.0f, (int)CollisionMask::eGround, (int)CollisionMask::ePlayer | (int)CollisionMask::eEnemy));
		}

		m_sphereRigidBody.resize(4);
		for (int i = 0; i < 4; i++) {

			m_sphereRigidBody[i].reset(new CollisionRigidBody(1.0f, 1.0f, (int)CollisionMask::eEnemy, (int)CollisionMask::eGround | (int)CollisionMask::ePlayer | (int)CollisionMask::eEnemy,
				glm::vec3(2.0f, (float)i * 2.0f + 10.0f, 0.0f)));
		}


		dynamicsWorld->setDebugDrawer(&m_collisionDebugDrawer);

		dynamicsWorld->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
	}

	resMgr.updateDescriptorHeap(&m_device);

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
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		1, 1
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

	m_blurXRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1
	);
	m_blurXRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1
	);
	m_blurXRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		0, 1
	);
	m_blurXRootSignature.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);

	m_blurYRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1
	);
	m_blurYRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1
	);
	m_blurYRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		0, 1
	);
	m_blurYRootSignature.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);

	ShaderSp blurxCS = resMgr.GetShader(m_blurXCs);
	ShaderSp bluryCS = resMgr.GetShader(m_blurYCs);

	m_blurXPass.setComputeShader(blurxCS->getByteCode());
	m_blurXPass.create(m_device.getDevice(), m_blurXRootSignature.getRootSignature());

	m_blurYPass.setComputeShader(bluryCS->getByteCode());
	m_blurYPass.create(m_device.getDevice(), m_blurYRootSignature.getRootSignature());

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

	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1);
	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		1, 1);
	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1);
	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 1);
	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		2, 1);
	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		3, 1);
	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		4, 1);
	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		5, 1);
	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		6, 1);
	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		7, 1);
	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		8, 1);
	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		0, 1);
	m_deferredRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		1, 1);

	m_deferredRootSignature.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	ShaderSp deferredps = resMgr.GetShader(m_deferredPs);

	m_deferredPass.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_deferredPass.setBlendState(BlendState::eNone);
	m_deferredPass.setDepthState(false, D3D12_COMPARISON_FUNC_LESS);
	m_deferredPass.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_deferredPass.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);
	m_deferredPass.setStencilEnable(false);
	m_deferredPass.setVertexShader(screenvs->getByteCode());
	m_deferredPass.setPixelShader(deferredps->getByteCode());

	if (!m_deferredPass.create(m_device.getDevice(), m_deferredRootSignature.getRootSignature()))
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
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		7, 1
	);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		8, 1
	);
	m_writeResultRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
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
	m_writeResultPass.addRenderTargetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_writeResultPass.setBlendState(BlendState::eNone);
	m_writeResultPass.setDepthState(true, D3D12_COMPARISON_FUNC_LESS);
	m_writeResultPass.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_writeResultPass.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);
	m_writeResultPass.setStencilEnable(false);
	m_writeResultPass.setVertexShader(vs->getByteCode());
	m_writeResultPass.setPixelShader(ps->getByteCode());

	if (!m_writeResultPass.create(m_device.getDevice(), m_writeResultRootSignature.getRootSignature()))
		return false;

	ShaderSp lightCullCS = resMgr.GetShader(m_lightCullCs);
	m_lightCullRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1);
	m_lightCullRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1);
	m_lightCullRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 1);
	m_lightCullRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		0, 1);
	m_lightCullRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		1, 1);

	m_lightCullRootSignature.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);


	m_lightCullPipeline.setComputeShader(lightCullCS->getByteCode());
	m_lightCullPipeline.create(m_device.getDevice(), m_lightCullRootSignature.getRootSignature());


	m_lineRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1);
	m_lineRootSignature.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1);
	m_lineRootSignature.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	ShaderSp lineVS = resMgr.GetShader(m_lineVs);
	ShaderSp linePS = resMgr.GetShader(m_linePs);

	m_linePass.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_linePass.setBlendState(BlendState::eNone);
	m_linePass.setDepthState(false, D3D12_COMPARISON_FUNC_LESS);
	m_linePass.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_linePass.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);
	m_linePass.setStencilEnable(false);
	m_linePass.setPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
	m_linePass.setVertexShader(lineVS->getByteCode());
	m_linePass.setPixelShader(linePS->getByteCode());

	if (!m_linePass.create(m_device.getDevice(), m_lineRootSignature.getRootSignature()))
		return false;


	return true;
}

void RenderPipeline::updateConstantBuffers(UINT curImageCount) {
	
	auto& resMgr = ResourceManager::Instance();
	const float scl = 0.25f;

	{
		Camera* camera = m_player.camera();
		ConstantBuffer* cb = resMgr.getResourceAsCB(m_viewProjBuffer);
		struct cb_t {
			glm::mat4 v;
			glm::mat4 p;
			glm::mat4 p1;
			glm::mat4 invVP;
		};
		cb_t source{};
		source.v = (camera->viewMatrix());
		source.p = (camera->projMatrix());
		source.p1 = glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f) * scl);
		source.invVP = glm::inverse(source.p * source.v);
		cb->updateBuffer(curImageCount, (UINT)sizeof(glm::mat4) * 4, &source);
	}
	{
		Camera* camera = m_player.camera();
		m_shadowcamera->Update();
		ConstantBuffer* cb = resMgr.getResourceAsCB(m_shadowVPBuffer);
		struct cb_t {
			glm::mat4 v;
			glm::mat4 p;
			glm::mat4 p1;
			glm::mat4 invVP;
			glm::mat4 invV;
			glm::vec3 lightVec;
		};
		cb_t source{};
		source.v = (m_shadowcamera->viewMatrix());
		source.p = (m_shadowcamera->projMatrix());
		source.p1 = glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f) * scl);
		source.invVP = glm::inverse(source.p * source.v);
		source.invV = glm::inverse(camera->viewMatrix());
		source.lightVec = m_shadowcamera->transform().forward();
		cb->updateBuffer(curImageCount, sizeof(cb_t), &source);
	}
	{
		ConstantBuffer* cb = resMgr.getResourceAsCB(m_screenInfoBuffer);
		glm::vec2 screen((float)kScreenWidth, (float)kScreenHeight);
		cb->updateBuffer(curImageCount, sizeof(glm::vec2), &screen);
	}
	{
		ConstantBuffer* cb = resMgr.getResourceAsCB(m_blurInfoBuffer);
		glm::vec3 screen(0.25f, (float)SHADOW_MAP_SIZE, (float)SHADOW_MAP_SIZE);
		cb->updateBuffer(curImageCount, sizeof(glm::vec3), &screen);
	}
	{
		Camera* camera = m_player.camera();
		StructuredBuffer* sb = resMgr.getResourceAsStuructured(m_lineBuffer);
		sb->updateBuffer(curImageCount, sizeof(CollisionDebugDraw::LineInfo) * m_collisionDebugDrawer.getLineCount(), (void*)(m_collisionDebugDrawer.getLineBuffer().data()));

		ConstantBuffer* cb = resMgr.getResourceAsCB(m_transposeVPBuffer);
		struct cb_t {
			glm::mat4 v;
			glm::mat4 p;
			glm::mat4 p1;
			glm::mat4 invVP;
		};
		cb_t source{};
		source.v = glm::transpose(camera->viewMatrix());
		source.p = glm::transpose(camera->projMatrix());
		source.p1 = glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f) * scl);
		source.invVP = glm::inverse(source.p * source.v);
		cb->updateBuffer(curImageCount, (UINT)sizeof(glm::mat4) * 4, &source);
	}
}



void RenderPipeline::writeResultBackBuffer(ID3D12GraphicsCommandList* command, UINT curImageCount) {

	//Model* sponzaModel = MeshManager::instance().getModel("models/SciFiHelmet/gltf/SciFiHelmet.gltf");
	Model* sponzaModel = MeshManager::instance().getModel("models/gltf/sponza.gltf");
	//Model* cubeModel = MeshManager::instance().getModel("models/sphere.gltf");
	Model* cubeModel = MeshManager::instance().getModel("models/cube.gltf");

	auto& resMgr = ResourceManager::Instance();
	
	{
		StructuredBuffer* lightBuffer = resMgr.getResourceAsStuructured(m_pointLightBuffer);
		StructuredBuffer* lightOffsetBuffer = resMgr.getResourceAsStuructured(m_lightIndexOffsetBuffer);
		StructuredBuffer* lightIndexBuffer = resMgr.getResourceAsStuructured(m_lightIndexBuffer);

		lightBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		lightOffsetBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		lightIndexBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);


		auto viewProjCbvHandle = resMgr.getShaderResourceGpuHandle(m_viewProjBuffer, 0);
		auto lightBufferHandle = resMgr.getShaderResourceGpuHandle(m_pointLightBuffer,  0);
		auto frustumBufferHandle = resMgr.getShaderResourceGpuHandle(m_frustumBuffer, 0);
		auto indexOffsetBufferHandle = resMgr.getShaderResourceGpuHandle(m_lightIndexOffsetBuffer, 1);
		auto lightIndexBufferHandle = resMgr.getShaderResourceGpuHandle(m_lightIndexBuffer, 1);

		command->SetComputeRootSignature(m_lightCullRootSignature.getRootSignature());

		command->SetPipelineState(m_lightCullPipeline.getPipelineState());

		command->SetComputeRootDescriptorTable(0, viewProjCbvHandle);
		command->SetComputeRootDescriptorTable(1, lightBufferHandle);
		command->SetComputeRootDescriptorTable(2, frustumBufferHandle);
		command->SetComputeRootDescriptorTable(3, indexOffsetBufferHandle);
		command->SetComputeRootDescriptorTable(4, lightIndexBufferHandle);

		command->Dispatch(1, 1, 16);

		lightBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		lightOffsetBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		lightIndexBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	{
		Texture* shadowMap = static_cast<Texture*>(resMgr.getResource(m_shadowMap));
		Texture* reflectMap = static_cast<Texture*>(resMgr.getResource(m_reflectMap));
		Texture* depthBuffer = static_cast<Texture*>(resMgr.getResource(m_shadowDepthBuffer));
		Texture* shadowBlurXBuffer = static_cast<Texture*>(resMgr.getResource(m_shadowBlurXBuffer));
		Texture* shadowBlurYBuffer = static_cast<Texture*>(resMgr.getResource(m_shadowBlurYBuffer));

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
			auto shadowCbvHandle = resMgr.getShaderResourceGpuHandle(m_viewProjBuffer, curImageCount);
			auto positionBufferSrv = resMgr.getShaderResourceGpuHandle(sponzaModel->getPositionBuffer(), 0);
			auto texcoordBufferSrv = resMgr.getShaderResourceGpuHandle(sponzaModel->getUVBuffer(), 0);
			auto indexBufferSrv = resMgr.getShaderResourceGpuHandle(sponzaModel->getIndexBuffer(), 0);
			auto indexOffsetHandle = resMgr.getShaderResourceGpuHandle(sponzaModel->getOffsetBuffer(), 0);
			auto samplerHandle = resMgr.getSamplerStateGpuHandle(m_wrapSampler, 0);

			command->SetGraphicsRootDescriptorTable(0, cbvHandle);
			command->SetGraphicsRootDescriptorTable(1, shadowCbvHandle);
			command->SetGraphicsRootDescriptorTable(2, positionBufferSrv);
			command->SetGraphicsRootDescriptorTable(3, texcoordBufferSrv);
			command->SetGraphicsRootDescriptorTable(4, indexBufferSrv);
			command->SetGraphicsRootDescriptorTable(5, indexOffsetHandle);
			command->SetGraphicsRootDescriptorTable(7, samplerHandle);

			command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			int indexOffset = 0;
			for (int i = 0; i < sponzaModel->getMaterialCount(); i++) {
				auto albedoTexHandle = resMgr.getShaderResourceGpuHandle(sponzaModel->getAlbedoTexture(i), 0);
				command->SetGraphicsRootDescriptorTable(6, albedoTexHandle);

				command->DrawInstanced(sponzaModel->getIndexCount(i), i + 1, 0, i);

				indexOffset += sponzaModel->getIndexCount(i);
			}
		}

		shadowBlurXBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		shadowBlurYBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		shadowMap->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		{
			auto blurInfoHandle = resMgr.getShaderResourceGpuHandle(m_blurInfoBuffer, curImageCount);
			auto sourceTexHandle = resMgr.getShaderResourceGpuHandle(m_shadowMap, 0);
			auto destinationTexHandle = resMgr.getShaderResourceGpuHandle(m_shadowBlurXBuffer, 1);

			command->SetPipelineState(m_blurXPass.getPipelineState());
			command->SetComputeRootSignature(m_blurXRootSignature.getRootSignature());

			command->SetComputeRootDescriptorTable(0, blurInfoHandle);
			command->SetComputeRootDescriptorTable(1, sourceTexHandle);
			command->SetComputeRootDescriptorTable(2, destinationTexHandle);

			command->Dispatch(SHADOW_MAP_SIZE / 256, SHADOW_MAP_SIZE, 1);
		}

		shadowBlurXBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		{
			auto blurInfoHandle = resMgr.getShaderResourceGpuHandle(m_blurInfoBuffer, curImageCount);
			auto sourceTexHandle = resMgr.getShaderResourceGpuHandle(m_shadowBlurXBuffer, 1);
			auto destinationTexHandle = resMgr.getShaderResourceGpuHandle(m_shadowBlurYBuffer, 1);

			command->SetPipelineState(m_blurYPass.getPipelineState());
			command->SetComputeRootSignature(m_blurYRootSignature.getRootSignature());

			command->SetComputeRootDescriptorTable(0, blurInfoHandle);
			command->SetComputeRootDescriptorTable(1, sourceTexHandle);
			command->SetComputeRootDescriptorTable(2, destinationTexHandle);

			command->Dispatch(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE / 256, 1);
		}
		shadowBlurYBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		shadowMap->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		reflectMap->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		depthBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	{
		Texture* backBuffer = static_cast<Texture*>(resMgr.getResource(m_backbuffer));
		Texture* fxaaBuffer = static_cast<Texture*>(resMgr.getResource(m_fxaaTexture));
		Texture* tempBuffer = static_cast<Texture*>(resMgr.getResource(m_tempTexture));
		Texture* depthBuffer = static_cast<Texture*>(resMgr.getResource(m_depthBuffer));
		Texture* albedoTexture = static_cast<Texture*>(resMgr.getResource(m_albedoTexture));
		Texture* normalTexture = static_cast<Texture*>(resMgr.getResource(m_normalTexture));
		Texture* roughMetalTexture = static_cast<Texture*>(resMgr.getResource(m_roughMetalTexture));

		albedoTexture->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		normalTexture->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		roughMetalTexture->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
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

		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[] = {
				resMgr.getRenderTargetCpuHandle(m_albedoTexture, 0),
				resMgr.getRenderTargetCpuHandle(m_normalTexture, 0),
				resMgr.getRenderTargetCpuHandle(m_roughMetalTexture, 0)
			};

			auto dsvHandle = resMgr.getDepthStencilCpuHandle(m_depthBuffer, curImageCount);
			command->OMSetRenderTargets(1, rtvHandle, false, &dsvHandle);
			float clearcolor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			command->ClearRenderTargetView(rtvHandle[0], clearcolor, 0, nullptr);
			command->ClearRenderTargetView(rtvHandle[1], clearcolor, 0, nullptr);
			command->ClearRenderTargetView(rtvHandle[2], clearcolor, 0, nullptr);
			command->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


			command->SetPipelineState(m_cubemapPass.getPipelineState());

			command->SetGraphicsRootSignature(m_cubemapRootSignature.getRootSignature());
		
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


			command->OMSetRenderTargets(3, rtvHandle, false, &dsvHandle);
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
			auto lightBufferHandle = resMgr.getShaderResourceGpuHandle(m_pointLightBuffer, 0);

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


		albedoTexture->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		normalTexture->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		roughMetalTexture->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		depthBuffer->transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		{
			auto albedoTexHandle = resMgr.getShaderResourceGpuHandle(m_albedoTexture, 0);
			auto normalTexHandle = resMgr.getShaderResourceGpuHandle(m_normalTexture, 0);
			auto roughMetalTexHandle = resMgr.getShaderResourceGpuHandle(m_roughMetalTexture, 0);
			auto shadowMapHandle = resMgr.getShaderResourceGpuHandle(m_shadowBlurYBuffer, 0);
			auto depthBufferHandle = resMgr.getShaderResourceGpuHandle(m_depthBuffer, curImageCount);
			auto reflectMapHandle = resMgr.getShaderResourceGpuHandle(m_reflectMap, 0);
			auto samplerHandle = resMgr.getSamplerStateGpuHandle(m_wrapSampler, 0);
			auto clampsamplerHandle = resMgr.getSamplerStateGpuHandle(m_clampSampler, 0);
			auto cameraBufferHandle = resMgr.getShaderResourceGpuHandle(m_viewProjBuffer, curImageCount);
			auto shadowBufferHandle = resMgr.getShaderResourceGpuHandle(m_shadowVPBuffer, curImageCount);
			auto pointLightBufferHandle = resMgr.getShaderResourceGpuHandle(m_pointLightBuffer, 0);
			auto indexOffsetBufferHandle = resMgr.getShaderResourceGpuHandle(m_lightIndexOffsetBuffer, 0);
			auto lightIndexBufferHandle = resMgr.getShaderResourceGpuHandle(m_lightIndexBuffer, 0);

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[] = {
				resMgr.getRenderTargetCpuHandle(m_fxaaTexture, 0)
			};

			command->OMSetRenderTargets(1, rtvHandle, false, nullptr);

			command->SetPipelineState(m_deferredPass.getPipelineState());
			command->SetGraphicsRootSignature(m_deferredRootSignature.getRootSignature());

			command->SetGraphicsRootDescriptorTable(0, cameraBufferHandle);
			command->SetGraphicsRootDescriptorTable(1, shadowBufferHandle);
			command->SetGraphicsRootDescriptorTable(2, albedoTexHandle);
			command->SetGraphicsRootDescriptorTable(3, normalTexHandle);
			command->SetGraphicsRootDescriptorTable(4, roughMetalTexHandle);
			command->SetGraphicsRootDescriptorTable(5, depthBufferHandle);
			command->SetGraphicsRootDescriptorTable(6, shadowMapHandle);
			command->SetGraphicsRootDescriptorTable(7, reflectMapHandle);
			command->SetGraphicsRootDescriptorTable(8, pointLightBufferHandle);
			command->SetGraphicsRootDescriptorTable(9, indexOffsetBufferHandle);
			command->SetGraphicsRootDescriptorTable(10, lightIndexBufferHandle);
			command->SetGraphicsRootDescriptorTable(11, samplerHandle);
			command->SetGraphicsRootDescriptorTable(12, clampsamplerHandle);

			command->DrawInstanced(4, 1, 0, 0);
		}

		fxaaBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		tempBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

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
			//auto sourceTexHandle = resMgr.getShaderResourceGpuHandle(m_reflectMap, 0);
			auto samplerHandle = resMgr.getSamplerStateGpuHandle(m_wrapSampler, 0);

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[] = {
				resMgr.getRenderTargetCpuHandle(m_backbuffer, curImageCount)
			};

			rtvHandle[0] = resMgr.getRenderTargetCpuHandle(m_backbuffer, curImageCount);
			command->OMSetRenderTargets(1, rtvHandle, false, nullptr);

			command->SetPipelineState(m_screenPass.getPipelineState());
			command->SetGraphicsRootSignature(m_screenRootSignature.getRootSignature());

			command->SetGraphicsRootDescriptorTable(0, sourceTexHandle);
			command->SetGraphicsRootDescriptorTable(1, samplerHandle);

			command->DrawInstanced(4, 1, 0, 0);
		}
		{
			auto lineBufferHandle = resMgr.getShaderResourceGpuHandle(m_lineBuffer, curImageCount);
			auto vpBufferHandle = resMgr.getShaderResourceGpuHandle(m_viewProjBuffer, curImageCount);


			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[] = {
				resMgr.getRenderTargetCpuHandle(m_backbuffer, curImageCount)
			};

			rtvHandle[0] = resMgr.getRenderTargetCpuHandle(m_backbuffer, curImageCount);
			command->OMSetRenderTargets(1, rtvHandle, false, nullptr);

			command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

			command->SetPipelineState(m_linePass.getPipelineState());
			command->SetGraphicsRootSignature(m_lineRootSignature.getRootSignature());

			command->SetGraphicsRootDescriptorTable(0, vpBufferHandle);
			command->SetGraphicsRootDescriptorTable(1, lineBufferHandle);

			command->DrawInstanced(m_collisionDebugDrawer.getLineCount(), 1, 0, 0);
		}

		m_gui.renderFrame(command);

		backBuffer->transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}
}