#ifndef _RENDER_PIPELINE_H_
#define _RENDER_PIPELINE_H_

#include "render_pass/render_pass.h"
#include "tools/model.h"
#include "object/camera.h"
#include "object/shadowcamera.h"
#include "object/cubecamera.h"
#include "framework/command_signature.h"
#include "tools/my_gui.h"

#include "tools/sh_calculator.h"

#include "object/collision/collision_manager.h"
#include "object/collision/collision_debug_drawer.h"

#include "object/player/player.h"

class RenderPipeline {
public:
	RenderPipeline() = default;
	~RenderPipeline() = default;


	bool initialize(HWND hwnd);
	void destroy();

	void render();


private:

	bool initializeResource();

	bool initializeResultPass();

	void updateConstantBuffers(UINT curImageCount);

	void writeResultBackBuffer(ID3D12GraphicsCommandList* command, UINT curImageCount);

	RootSignature m_cubemapRS;
	RootSignature m_visibilityRS;
	RootSignature m_writeResultRS;

	RootSignature m_shadowmapRS;

	RootSignature m_fxaaRS;
	RootSignature m_screenRS;

	RootSignature m_lightCullRS;

	RootSignature m_clusterCullRS;

	RootSignature m_materialTestRS;

	RootSignature m_deferredRS;


	Pipeline m_visibilityPipeline;

	Pipeline m_screenPipeline;

	ComputePipeline m_clusterCullPipeline;
	ComputePipeline m_materialTestPipeline;
	ComputePipeline m_fxaaPipeline;

	ComputePipeline m_lightCullPipeline;

	ComputePipeline m_deferredPipeline;

	CommandSignature m_drawCommandSignature;
	CommandSignature m_dispatchCommandSignature;

	std::vector<CommandAllocator> m_commandAllocator;
	std::vector<CommandList> m_commandList;
	Swapchain m_swapchain;

	Fence m_fence;

	Device m_device;
	Queue m_queue;

	int m_backbuffer;
	int m_depthBuffer;

	int m_visibilityBuffer;

	int m_tempTexture;
	int m_fxaaTexture;

	int m_cubeMap;

	int m_wrapSampler;
	int m_clampSampler;

	int m_viewProjBuffer;
	int m_materialIdBuffer;
	int m_deferredCB;

	int m_countBuffer;
	int m_drawArgsBuffer[2];
	int m_tempCopyBuffer;
	int m_tempCopyBuffer2;

	int m_counterBuffer[2];
	int m_indexOffsetBuffer[2];

	int m_dispatchIndirectBuffer[2];

	int m_pointLightBuffer;
	int m_lightIndexOffsetBuffer;
	int m_lightIndexBuffer;

	int m_frustumBuffer;
	int m_cameraFrustumPlaneBuffer;

	int m_materialBuffer;

	int m_clusterCullCS;

	int m_materialTestCS;

	int m_visibilityVS;
	int m_visibilityPS;

	int m_deferredCS;

	int m_screenVS;
	int m_screenPS;

	int m_fxaaCS;
	int m_lightCullCS;

	std::unique_ptr<Camera> m_camera2;
	std::unique_ptr<ShadowCamera> m_shadowcamera;
	std::unique_ptr<CubeCamera> m_cubecamera;

	CollisionDebugDraw m_collisionDebugDrawer;

	Player m_player;

	std::unique_ptr<CollisionRigidBody> m_groundRigidBody;
	std::vector<std::unique_ptr<CollisionRigidBody>> m_sphereRigidBody;

	int m_frame = 0;

	MyGui m_gui;

	bool m_cameraLock = false;
	bool m_isVisibilityBuffer = false;

	static const int CUBE_MAP_SIZE = 256;
	static const int SHADOW_MAP_SIZE = 2048;


	static const int FRUSTUM_SIZE_X = 16;
	static const int FRUSTUM_SIZE_Y = 16;
	static const int FRUSTUM_SIZE_Z = 16;

	static const int DRAW_COMMAND_COUNT = 2048;
};

#endif