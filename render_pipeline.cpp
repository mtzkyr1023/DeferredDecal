#include "render_pipeline.h"
#include "defines.h"
#include "tools/input.h"
#include "imgui_dx12/imgui.h"
#include "imgui_dx12/imgui_impl_win32.h"
#include "imgui_dx12/imgui_impl_dx12.h"

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

	m_gui.create(hwnd, m_device.getDevice(), DXGI_FORMAT_R8G8B8A8_UNORM, kBackBufferCount, ResourceManager::Instance().getGlobalHeap()->getDescriptorHeap());

	
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


		ImGui::Checkbox("Camera Lock", &m_cameraLock);

		ImGui::Checkbox("Show VisibilityBuffer", &m_isVisibilityBuffer);
		{
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
				resMgr.getGlobalHeap()->getDescriptorHeap(),
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

	m_visibilityBuffer = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, DXGI_FORMAT_R32G32_UINT, kScreenWidth, kScreenHeight);

	m_fxaaTexture = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		DXGI_FORMAT_R16G16B16A16_FLOAT, kScreenWidth, kScreenHeight);
	m_tempTexture = resMgr.createRenderTarget2D(m_device.getDevice(), 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		DXGI_FORMAT_R16G16B16A16_FLOAT, kScreenWidth, kScreenHeight);

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

	m_cameraFrustumPlaneBuffer = resMgr.createConstantBuffer(m_device.getDevice(), sizeof(glm::mat4) * 4, kBackBufferCount);

	m_visibilityVS = resMgr.addVertexShader(L"shaders/visibility_vs.fx");
	m_visibilityPS = resMgr.addPixelShader(L"shaders/visibility_ps.fx");

	m_clusterCullCS = resMgr.addComputeShader(L"shaders/create_vertex_cs.fx");
	m_materialTestCS = resMgr.addComputeShader(L"shaders/material_test_cs.fx");

	m_screenVS = resMgr.addVertexShader(L"shaders/screen_vs.fx");
	m_screenPS = resMgr.addPixelShader(L"shaders/last_ps.fx");

	m_fxaaCS = resMgr.addComputeShader(L"shaders/fxaa_cs.fx");

	m_lightCullCS = resMgr.addComputeShader(L"shaders/clustered_cull_cs.fx");

	m_deferredCS = resMgr.addComputeShader(L"shaders/deferred_cs.fx");

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

	m_viewProjBuffer = resMgr.createConstantBuffer(m_device.getDevice(), (UINT)sizeof(glm::mat4) * 8, kBackBufferCount);
	m_materialIdBuffer = resMgr.createConstantBuffer(m_device.getDevice(), (UINT)sizeof(glm::mat4) * 4, kBackBufferCount);
	m_deferredCB = resMgr.createConstantBuffer(m_device.getDevice(), (UINT)sizeof(glm::mat4) * 4, kBackBufferCount);

	m_drawArgsBuffer[0] = resMgr.createAppendStructuredBuffer(m_device.getDevice(), 1, (UINT)sizeof(D3D12_DRAW_INDEXED_ARGUMENTS) + sizeof(uint64_t), DRAW_COMMAND_COUNT, false, true);
	m_drawArgsBuffer[1] = resMgr.createAppendStructuredBuffer(m_device.getDevice(), 1, (UINT)sizeof(D3D12_DRAW_INDEXED_ARGUMENTS) + sizeof(uint64_t), DRAW_COMMAND_COUNT, false, true);
	
	{
		struct tempStruct{
			uint64_t temp;
			D3D12_DRAW_INDEXED_ARGUMENTS temp2;
		};

		tempStruct drawArgs[DRAW_COMMAND_COUNT];
		memset(drawArgs, 0, sizeof(tempStruct) * DRAW_COMMAND_COUNT);
		m_tempCopyBuffer = resMgr.createStructuredBuffer(m_device.getDevice(), m_queue.getQueue(), 1, (UINT)sizeof(D3D12_DRAW_INDEXED_ARGUMENTS) + sizeof(uint64_t), DRAW_COMMAND_COUNT, drawArgs);
	}
	UINT clearValue = 0;
	m_tempCopyBuffer2 = resMgr.createStructuredBuffer(m_device.getDevice(), m_queue.getQueue(), 1, (UINT)sizeof(UINT), 1, &clearValue);
	m_indexOffsetBuffer[0] = resMgr.createAppendStructuredBuffer(m_device.getDevice(), 1, (UINT)sizeof(UINT), DRAW_COMMAND_COUNT, false, true);
	m_indexOffsetBuffer[1] = resMgr.createAppendStructuredBuffer(m_device.getDevice(), 1, (UINT)sizeof(UINT), DRAW_COMMAND_COUNT, false, true);
	m_counterBuffer[0] = resMgr.createByteAddressBuffer(m_device.getDevice(), DXGI_FORMAT_R32_UINT, 1, sizeof(UINT), true);
	m_counterBuffer[1] = resMgr.createByteAddressBuffer(m_device.getDevice(), DXGI_FORMAT_R32_UINT, 1, sizeof(UINT), true);

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
	//{
	//	Model* model = MeshManager::instance().getModel("models/gltf/sponza.gltf");
	//	struct DispatchIndirect {
	//		uint64_t albedoBuffer;
	//	};
	//	std::vector<DispatchIndirect> materialInfo(model->getMaterialCount());
	//	for (int i = 0; i < model->getMaterialCount(); i++) {
	//		materialInfo[i].albedoBuffer = resMgr.getResource(model->getAlbedoTexture(i))->getResource(0)->GetGPUVirtualAddress();
	//	}
	//	m_materialBuffer = resMgr.createStructuredBuffer(m_device.getDevice(), m_queue.getQueue(), 1, sizeof(DispatchIndirect), model->getMaterialCount(), materialInfo.data());
	//}
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

	resMgr.updateDescriptorHeap(&m_device, 1024);

	return true;
}


bool RenderPipeline::initializeResultPass() {

	auto& resMgr = ResourceManager::Instance();

	m_clusterCullRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1
	);
	m_clusterCullRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		1, 1
	);
	m_clusterCullRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1
	);
	m_clusterCullRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		0, 1
	);
	m_clusterCullRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		1, 1
	);
	m_clusterCullRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		2, 1
	);

	m_clusterCullRS.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	ShaderSp clusterCullCS = resMgr.GetShader(m_clusterCullCS);

	m_clusterCullPipeline.setComputeShader(clusterCullCS->getByteCode());
	m_clusterCullPipeline.create(m_device.getDevice(), m_clusterCullRS.getRootSignature());

	m_materialTestRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1
	);
	m_materialTestRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		0, 1
	);

	m_materialTestRS.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	ShaderSp materialTestCS = resMgr.GetShader(m_materialTestCS);

	m_materialTestPipeline.setComputeShader(materialTestCS->getByteCode());
	m_materialTestPipeline.create(m_device.getDevice(), m_materialTestRS.getRootSignature());

	m_visibilityRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1
	);
	m_visibilityRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		1, 1, D3D12_ROOT_PARAMETER_TYPE_CBV
	);
	m_visibilityRS.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);


	ShaderSp visibilityvs = resMgr.GetShader(m_visibilityVS);
	ShaderSp visibilityps = resMgr.GetShader(m_visibilityPS);

	m_visibilityPipeline.addInputLayout("POSITION", DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
	m_visibilityPipeline.addInputLayout("NORMAL", DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
	m_visibilityPipeline.addInputLayout("TANGENT", DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
	m_visibilityPipeline.addInputLayout("TEXCOORD", DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
	m_visibilityPipeline.addRenderTargetFormat(DXGI_FORMAT_R32G32_UINT);
	m_visibilityPipeline.addRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_visibilityPipeline.setBlendState(BlendState::eNone);
	m_visibilityPipeline.setDepthState(true, D3D12_COMPARISON_FUNC_LESS_EQUAL);
	m_visibilityPipeline.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_visibilityPipeline.setStencilEnable(false);
	m_visibilityPipeline.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);
	m_visibilityPipeline.setVertexShader(visibilityvs->getByteCode());
	m_visibilityPipeline.setPixelShader(visibilityps->getByteCode());
	m_visibilityPipeline.setPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	m_visibilityPipeline.create(m_device.getDevice(), m_visibilityRS.getRootSignature());

	m_drawCommandSignature.addConstantBuffer(1);
	m_drawCommandSignature.addDrawIndexedCommand();

	m_drawCommandSignature.createDrawIndirect(m_device.getDevice(), m_visibilityRS.getRootSignature());

	m_screenRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1);
	m_screenRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_PIXEL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		0, 1);
	m_screenRS.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);


	ShaderSp screenvs = resMgr.GetShader(m_screenVS);
	ShaderSp screenps = resMgr.GetShader(m_screenPS);

	m_screenPipeline.addRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	m_screenPipeline.setBlendState(BlendState::eNone);
	m_screenPipeline.setDepthState(false, D3D12_COMPARISON_FUNC_LESS);
	m_screenPipeline.setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	m_screenPipeline.setRasterState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);
	m_screenPipeline.setStencilEnable(false);
	m_screenPipeline.setVertexShader(screenvs->getByteCode());
	m_screenPipeline.setPixelShader(screenps->getByteCode());

	if (!m_screenPipeline.create(m_device.getDevice(), m_screenRS.getRootSignature()))
		return false;



	m_fxaaRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1
	);
	m_fxaaRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1);
	m_fxaaRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		0, 1);
	m_fxaaRS.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);

	ShaderSp fxaaCs = resMgr.GetShader(m_fxaaCS);
	m_fxaaPipeline.setComputeShader(fxaaCs->getByteCode());
	m_fxaaPipeline.create(m_device.getDevice(), m_fxaaRS.getRootSignature());

	ShaderSp lightCullCS = resMgr.GetShader(m_lightCullCS);
	m_lightCullRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1);
	m_lightCullRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1);
	m_lightCullRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 1);
	m_lightCullRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		0, 1);
	m_lightCullRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		1, 1);

	m_lightCullRS.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);


	m_lightCullPipeline.setComputeShader(lightCullCS->getByteCode());
	m_lightCullPipeline.create(m_device.getDevice(), m_lightCullRS.getRootSignature());


	m_deferredRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		0, 1);
	m_deferredRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1);
	m_deferredRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 1);
	m_deferredRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		2, 1);
	m_deferredRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		3, 1);
	m_deferredRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		4, 1);
	m_deferredRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		5, 1);
	m_deferredRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
		0, 1);
	m_deferredRS.addDescriptorCount(
		D3D12_SHADER_VISIBILITY_ALL,
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		0, 1);

	m_deferredRS.create(m_device.getDevice(),
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);

	ShaderSp deferredCS = resMgr.GetShader(m_deferredCS);

	m_deferredPipeline.setComputeShader(deferredCS->getByteCode());

	m_deferredPipeline.create(m_device.getDevice(), m_deferredRS.getRootSignature());

	m_dispatchCommandSignature.addShaderResource(4);
	m_dispatchCommandSignature.addDispatchCommand();

	//m_dispatchCommandSignature.createDispatchIndirect(m_device.getDevice(), m_deferredRS.getRootSignature());

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
			glm::vec4 camPos;
			glm::mat4 padding1[3];
			glm::mat4x2 padding2;
			glm::vec4 screenInfo;
		};
		cb_t source{};
		source.v = glm::transpose(camera->viewMatrix());
		source.p = glm::transpose(camera->projMatrix());
		source.p1 = glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f) * scl);
		source.camPos = glm::vec4(camera->transform().position(), 1.0f);
		source.invVP = glm::inverse(source.p * source.v);
		source.screenInfo = glm::vec4((float)kScreenWidth, (float)kScreenHeight, camera->nearZ(), camera->farZ());
		cb->updateBuffer(curImageCount, (UINT)sizeof(glm::mat4) * 8, &source);
	}
	{
		Camera* camera = m_player.camera();
		ConstantBuffer* cb = resMgr.getResourceAsCB(m_deferredCB);
		struct cb_t {
			glm::mat4 invVP;
			glm::mat4 worldViewMatrix;
			glm::vec4 cameraPos;
			glm::uvec2 imageSize;
			float isVB;
		};

		cb_t source{};
		source.invVP = glm::transpose(glm::inverse(camera->projMatrix()));
		source.worldViewMatrix = glm::transpose(camera->viewMatrix() * glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f) * scl));
		source.cameraPos = glm::vec4(camera->transform().position(), 0.0f);
		source.imageSize = glm::uvec2(kScreenWidth, kScreenHeight);
		source.isVB = m_isVisibilityBuffer ? 1.0f : -1.0f;


		cb->updateBuffer(curImageCount, sizeof(cb_t), &source);
	}
	{
		Camera* camera = m_player.camera();
		ConstantBuffer* cb = resMgr.getResourceAsCB(m_cameraFrustumPlaneBuffer);

		struct cb_t {
			glm::vec4 plane[6];
			glm::mat4 viewMatrix;
		};
		glm::mat4 viewProj = (camera->projMatrix() * camera->viewMatrix());
		cb_t source{};
		source.plane[0][0] = viewProj[0][3] + viewProj[0][0];
		source.plane[0][1] = viewProj[1][3] + viewProj[1][0];
		source.plane[0][2] = viewProj[2][3] + viewProj[2][0];
		source.plane[0][3] = viewProj[3][3] + viewProj[3][0];

		source.plane[1][0] = viewProj[0][3] - viewProj[0][0];
		source.plane[1][1] = viewProj[1][3] - viewProj[1][0];
		source.plane[1][2] = viewProj[2][3] - viewProj[2][0];
		source.plane[1][3] = viewProj[3][3] - viewProj[3][0];

		source.plane[2][0] = viewProj[0][3] - viewProj[0][1];
		source.plane[2][1] = viewProj[1][3] - viewProj[1][1];
		source.plane[2][2] = viewProj[2][3] - viewProj[2][1];
		source.plane[2][3] = viewProj[3][3] - viewProj[3][1];

		source.plane[3][0] = viewProj[0][3] + viewProj[0][1];
		source.plane[3][1] = viewProj[1][3] + viewProj[1][1];
		source.plane[3][2] = viewProj[2][3] + viewProj[2][1];
		source.plane[3][3] = viewProj[3][3] + viewProj[3][1];

		source.plane[4][0] = viewProj[0][3] + viewProj[0][2];
		source.plane[4][1] = viewProj[1][3] + viewProj[1][2];
		source.plane[4][2] = viewProj[2][3] + viewProj[2][2];
		source.plane[4][3] = viewProj[3][3] + viewProj[3][2];

		source.plane[5][0] = viewProj[0][3] - viewProj[0][2];
		source.plane[5][1] = viewProj[1][3] - viewProj[1][2];
		source.plane[5][2] = viewProj[2][3] - viewProj[2][2];
		source.plane[5][3] = viewProj[3][3] - viewProj[3][2];

		source.viewMatrix = glm::transpose(camera->viewMatrix());

		for (int i = 0; i < 6; i++) {
			float len = glm::length(glm::vec3(source.plane[i][0], source.plane[i][1], source.plane[i][2]));
			source.plane[i].x /= len;
			source.plane[i].y /= len;
			source.plane[i].z /= len;
			source.plane[i].w /= len;
		}

		if (!m_cameraLock)
			cb->updateBuffer(curImageCount, sizeof(cb_t), &source);
	}
}



void RenderPipeline::writeResultBackBuffer(ID3D12GraphicsCommandList* command, UINT curImageCount) {

	//Model* sponzaModel = MeshManager::instance().getModel("models/SciFiHelmet/gltf/SciFiHelmet.gltf");
	Model* sponzaModel = MeshManager::instance().getModel("models/gltf/sponza.gltf");
	//Model* cubeModel = MeshManager::instance().getModel("models/sphere.gltf");
	//Model* cubeModel = MeshManager::instance().getModel("models/cube.gltf");

	static int heapIndex = 0;
	if (curImageCount % kBackBufferCount == 0) heapIndex = 0;

	auto& resMgr = ResourceManager::Instance();
	{

		StructuredBuffer* lightBuffer = resMgr.getResourceAsStuructured(m_pointLightBuffer);
		StructuredBuffer* lightOffsetBuffer = resMgr.getResourceAsStuructured(m_lightIndexOffsetBuffer);
		StructuredBuffer* lightIndexBuffer = resMgr.getResourceAsStuructured(m_lightIndexBuffer);

		lightBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		lightOffsetBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		lightIndexBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);


		auto viewProjCbvHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_viewProjBuffer, curImageCount);
		auto lightBufferHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_pointLightBuffer, 0);
		auto frustumBufferHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_frustumBuffer, 0);
		auto indexOffsetBufferHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_lightIndexOffsetBuffer, 1);
		auto lightIndexBufferHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_lightIndexBuffer, 1);

		command->SetComputeRootSignature(m_lightCullRS.getRootSignature());

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
		StructuredBuffer* clusterBuffer = static_cast<StructuredBuffer*>(resMgr.getResource(sponzaModel->getInstanceBuffer()));
		StructuredBuffer* drawArgBuffer = static_cast<StructuredBuffer*>(resMgr.getResource(m_drawArgsBuffer[curImageCount]));
		StructuredBuffer* indexOffsetBuffer = static_cast<StructuredBuffer*>(resMgr.getResource(m_indexOffsetBuffer[curImageCount]));
		ByteAddressBuffer* counterBuffer = static_cast<ByteAddressBuffer*>(resMgr.getResource(m_counterBuffer[curImageCount]));


		command->SetComputeRootSignature(m_clusterCullRS.getRootSignature());

		command->SetPipelineState(m_clusterCullPipeline.getPipelineState());

		auto cbvHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_viewProjBuffer, curImageCount);
		auto frustumHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_cameraFrustumPlaneBuffer, curImageCount);
		auto clusterHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, sponzaModel->getInstanceBuffer(), 0);
		auto drawIndirectHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_drawArgsBuffer[curImageCount], 1);
		auto indexOffsetHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_indexOffsetBuffer[curImageCount], 1);
		auto counterHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_counterBuffer[curImageCount], 1);

#if 1
		drawArgBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
		indexOffsetBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
		counterBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);

		int offset = drawArgBuffer->getCounterBufferOffset();
		command->CopyBufferRegion(resMgr.getResource(m_drawArgsBuffer[curImageCount])->getResource(0), offset, resMgr.getResource(m_tempCopyBuffer2)->getResource(0), 0, sizeof(UINT));
		offset = indexOffsetBuffer->getCounterBufferOffset();
		command->CopyBufferRegion(resMgr.getResource(m_indexOffsetBuffer[curImageCount])->getResource(0), offset, resMgr.getResource(m_tempCopyBuffer2)->getResource(0), 0, sizeof(UINT));
		offset = 0;
		command->CopyBufferRegion(resMgr.getResource(m_counterBuffer[curImageCount])->getResource(0), offset, resMgr.getResource(m_tempCopyBuffer2)->getResource(0), 0, sizeof(UINT));

		drawArgBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		indexOffsetBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		counterBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#else
		drawArgBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#endif

		command->SetComputeRootDescriptorTable(0, frustumHandle);
		command->SetComputeRootDescriptorTable(1, cbvHandle);
		command->SetComputeRootDescriptorTable(2, clusterHandle);
		command->SetComputeRootDescriptorTable(3, drawIndirectHandle);
		command->SetComputeRootDescriptorTable(4, indexOffsetHandle);
		command->SetComputeRootDescriptorTable(5, counterHandle);

		command->Dispatch(sponzaModel->getInstanceCount(), 1, 1);

		drawArgBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		indexOffsetBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	}
	{
		Texture* backBuffer = static_cast<Texture*>(resMgr.getResource(m_backbuffer));
		Texture* fxaaBuffer = static_cast<Texture*>(resMgr.getResource(m_fxaaTexture));
		Texture* tempBuffer = static_cast<Texture*>(resMgr.getResource(m_tempTexture));
		Texture* depthBuffer = static_cast<Texture*>(resMgr.getResource(m_depthBuffer));
		Texture* visibilityBuffer = static_cast<Texture*>(resMgr.getResource(m_visibilityBuffer));
		VertexBuffer* vertexBuffer = static_cast<VertexBuffer*>(resMgr.getResource(sponzaModel->getVertexBuffer()));
		IndexBuffer* indexBuffer = static_cast<IndexBuffer*>(resMgr.getResource(sponzaModel->getIndexBuffer()));
		StructuredBuffer* drawArgBuffer = static_cast<StructuredBuffer*>(resMgr.getResource(m_drawArgsBuffer[(curImageCount + 1) % kBackBufferCount]));

		fxaaBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		depthBuffer->transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		visibilityBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

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
				resMgr.getRenderTargetCpuHandle(m_visibilityBuffer, 0),
				resMgr.getRenderTargetCpuHandle(m_fxaaTexture, 0),
			};

			auto dsvHandle = resMgr.getDepthStencilCpuHandle(m_depthBuffer, curImageCount);
			command->OMSetRenderTargets(2, rtvHandle, false, &dsvHandle);
			float clearcolor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			command->ClearRenderTargetView(rtvHandle[0], clearcolor, 0, nullptr);
			command->ClearRenderTargetView(rtvHandle[1], clearcolor, 0, nullptr);
			command->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


			command->SetPipelineState(m_visibilityPipeline.getPipelineState());

			command->SetGraphicsRootSignature(m_visibilityRS.getRootSignature());
		
			auto cbv0Handle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_viewProjBuffer, curImageCount);
			auto cbv1Handle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_materialIdBuffer, 0);

			command->SetGraphicsRootDescriptorTable(0, cbv0Handle);
			//command->SetGraphicsRootDescriptorTable(1, cbv1Handle);

			command->IASetVertexBuffers(0, 1, vertexBuffer->getVertexBuferView(0));
			command->IASetIndexBuffer(indexBuffer->getIndexBufferView(0));
			command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			//command->DrawIndexedInstanced(sponzaModel->getAllIndexCount(), 1, 0, 0, 0);

			//int indexOffset = 0;
			//for (int i = 0; i < sponzaModel->getMaterialCount(); i++) {

			//	command->DrawIndexedInstanced(sponzaModel->getIndexCount(i), 1, indexOffset, 0, 0);

			//	indexOffset += sponzaModel->getIndexCount(i);
			//}

			command->ExecuteIndirect(m_drawCommandSignature.getCommandSignatue(), DRAW_COMMAND_COUNT,
				resMgr.getResource(m_drawArgsBuffer[(curImageCount + 1) % kBackBufferCount])->getResource(0), 0,
				resMgr.getResource(m_drawArgsBuffer[(curImageCount + 1) % kBackBufferCount])->getResource(0), drawArgBuffer->getCounterBufferOffset());

		}

		depthBuffer->transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		fxaaBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		visibilityBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		tempBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		//{

		//	auto visibility = resMgr.getShaderResourceGpuHandle(m_visibilityBuffer, 0);
		//	auto material = resMgr.getShaderResourceGpuHandle(m_materialOffsetBuffer, 1);

		//	materialBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		//	command->SetComputeRootSignature(m_materialTestRS.getRootSignature());

		//	command->SetPipelineState(m_materialTestPipeline.getPipelineState());

		//	command->SetComputeRootDescriptorTable(0, visibility);
		//	command->SetComputeRootDescriptorTable(1, material);

		//	command->Dispatch(16, 8, 1);

		//	materialBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		//}

		{
			auto cbv = resMgr.getGlobalHeap((heapIndex++) % 1024, m_deferredCB, curImageCount);
			auto visibility = resMgr.getGlobalHeap((heapIndex++) % 1024, m_visibilityBuffer, 0);
			auto vertex = resMgr.getGlobalHeap((heapIndex++) % 1024, sponzaModel->getVertexBuffer(), 0);
			auto index = resMgr.getGlobalHeap((heapIndex++) % 1024, sponzaModel->getIndexBuffer(), 0);
			auto indexOffset = resMgr.getGlobalHeap((heapIndex++) % 1024, sponzaModel->getInstanceBuffer(), 0);
			auto temp = resMgr.getGlobalHeap((heapIndex++) % 1024, m_fxaaTexture, 0);
			auto instanceToIndexMap = resMgr.getGlobalHeap((heapIndex++) % 1024, sponzaModel->instanceToIndexMap(), 0);
			auto albedo = resMgr.getGlobalHeap((heapIndex++) % 1024, sponzaModel->getAlbedoTexture(0), 0);
			for (int i = 1; i < sponzaModel->getMaterialCount(); i++) {
				resMgr.getGlobalHeap((heapIndex++) % 1024, sponzaModel->getAlbedoTexture(i), 0);
			}
			auto sampler = resMgr.getSamplerStateGpuHandle(m_wrapSampler, 0);

			command->SetPipelineState(m_deferredPipeline.getPipelineState());

			command->SetComputeRootSignature(m_deferredRS.getRootSignature());

			command->SetComputeRootDescriptorTable(0, cbv);
			command->SetComputeRootDescriptorTable(1, visibility);
			command->SetComputeRootDescriptorTable(2, vertex);
			command->SetComputeRootDescriptorTable(3, index);
			command->SetComputeRootDescriptorTable(4, indexOffset);
			command->SetComputeRootDescriptorTable(5, instanceToIndexMap);
			command->SetComputeRootDescriptorTable(6, albedo);
			command->SetComputeRootDescriptorTable(7, sampler);
			command->SetComputeRootDescriptorTable(8, temp);

			command->Dispatch(kScreenWidth / 16, kScreenHeight / 8, 1);
		}

		fxaaBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		{
			auto cbvHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_viewProjBuffer, curImageCount);
			auto sourceTexHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_fxaaTexture, 0);
			auto destTexHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_tempTexture, 0);
			command->SetPipelineState(m_fxaaPipeline.getPipelineState());

			command->SetComputeRootSignature(m_fxaaRS.getRootSignature());

			command->SetComputeRootDescriptorTable(0, cbvHandle);
			command->SetComputeRootDescriptorTable(1, sourceTexHandle);
			command->SetComputeRootDescriptorTable(2, destTexHandle);

			command->Dispatch(kScreenWidth / 16, kScreenHeight / 16, 1);
		}


		tempBuffer->transitionResource(command, 0, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		backBuffer->transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		{
			auto sourceTexHandle = resMgr.getGlobalHeap((heapIndex++) % 1024, m_tempTexture, 0);
			//auto sourceTexHandle = resMgr.getShaderResourceGpuHandle(m_reflectMap, 0);
			auto samplerHandle = resMgr.getSamplerStateGpuHandle(m_wrapSampler, 0);

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[] = {
				resMgr.getRenderTargetCpuHandle(m_backbuffer, curImageCount)
			};

			rtvHandle[0] = resMgr.getRenderTargetCpuHandle(m_backbuffer, curImageCount);
			command->OMSetRenderTargets(1, rtvHandle, false, nullptr);

			command->SetPipelineState(m_screenPipeline.getPipelineState());
			command->SetGraphicsRootSignature(m_screenRS.getRootSignature());

			command->SetGraphicsRootDescriptorTable(0, sourceTexHandle);
			command->SetGraphicsRootDescriptorTable(1, samplerHandle);

			command->DrawInstanced(4, 1, 0, 0);
		}

		m_gui.renderFrame(command);

		backBuffer->transitionResource(command, curImageCount, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}
}