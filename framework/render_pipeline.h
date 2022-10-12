#ifndef _RENDER_PIPELINE_H_
#define _RENDER_PIPELINE_H_

#include "../render_pass/render_pass.h"
#include "../tools/model.h"
#include "../object/camera.h"
#include "../object/shadowcamera.h"
#include "../object/cubecamera.h"
#include "../framework/command_signature.h"
#include "../tools/my_gui.h"

#include "../tools/sh_calculator.h"

#include "../object/collision/collision_manager.h"
#include "../object/collision/collision_debug_drawer.h"

#include "../object/player/player.h"

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

	RootSignature m_cubemapRootSignature;
	RootSignature m_simpleRootSignature;
	RootSignature m_writeResultRootSignature;

	RootSignature m_shadowMapRootSignature;

	RootSignature m_fxaaRootSignature;
	RootSignature m_screenRootSignature;

	RootSignature m_blurXRootSignature;
	RootSignature m_blurYRootSignature;

	RootSignature m_deferredRootSignature;

	RootSignature m_lightCullRootSignature;

	RootSignature m_lineRootSignature;
	
	Pipeline m_shadowMapPass;

	Pipeline m_cubemapPass;
	Pipeline m_simplePass;
	Pipeline m_writeResultPass;
	Pipeline m_linePass;
	Pipeline m_screenPass;

	ComputePipeline m_blurXPass;
	ComputePipeline m_blurYPass;

	Pipeline m_deferredPass;

	ComputePipeline m_fxaaPipeline;

	ComputePipeline m_lightCullPipeline;

	std::vector<CommandAllocator> m_commandAllocator;
	std::vector<CommandList> m_commandList;
	Swapchain m_swapchain;

	Fence m_fence;

	Device m_device;
	Queue m_queue;

	int m_testVs;
	int m_testPs;
	int m_simpleVs;
	int m_simplePs;
	int m_screenVs;
	int m_screenPs;

	int m_cubemapVs;
	int m_cubemapPs;

	int m_shadowVs;
	int m_shadowPs;

	int m_lineVs;
	int m_linePs;

	int m_blurXCs;
	int m_blurYCs;

	int m_lightCullCs;

	int m_deferredPs;

	int m_fxaaCs;

	int m_cubeMap;

	int m_backbuffer;
	int m_depthBuffer;

	int m_shadowMap;
	int m_reflectMap;
	int m_shadowDepthBuffer;

	int m_albedoTexture;
	int m_normalTexture;
	int m_roughMetalTexture;

	int m_fxaaTexture;

	int m_tempTexture;

	int m_shadowBlurXBuffer;
	int m_shadowBlurYBuffer;

	int m_pointLightBuffer;

	int m_lightIndexOffsetBuffer;
	int m_lightIndexBuffer;

	int m_lineBuffer;

	int m_blurInfoBuffer;

	int m_wrapSampler;
	int m_clampSampler;

	int m_viewProjBuffer;

	int m_shadowVPBuffer;

	int m_screenInfoBuffer;
	int m_transposeVPBuffer;

	int m_frustumBuffer;

	std::unique_ptr<Camera> m_camera2;
	std::unique_ptr<ShadowCamera> m_shadowcamera;
	std::unique_ptr<CubeCamera> m_cubecamera;

	CollisionDebugDraw m_collisionDebugDrawer;

	Player m_player;

	std::unique_ptr<CollisionRigidBody> m_groundRigidBody;
	std::vector<std::unique_ptr<CollisionRigidBody>> m_sphereRigidBody;

	int m_frame = 0;

	MyGui m_gui;

	static const int CUBE_MAP_SIZE = 256;
	static const int SHADOW_MAP_SIZE = 2048;


	static const int FRUSTUM_SIZE_X = 16;
	static const int FRUSTUM_SIZE_Y = 16;
	static const int FRUSTUM_SIZE_Z = 16;
};

#endif