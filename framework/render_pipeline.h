#ifndef _RENDER_PIPELINE_H_
#define _RENDER_PIPELINE_H_

#include "../render_pass/render_pass.h"
#include "../tools/model.h"
#include "../object/camera.h"
#include "../object/shadowcamera.h"
#include "../object/cubecamera.h"
#include "../framework/command_signature.h"

#include "../tools/sh_calculator.h"

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
	
	Pipeline m_shadowMapPass;

	Pipeline m_cubemapPass;
	Pipeline m_simplePass;
	Pipeline m_writeResultPass;
	Pipeline m_screenPass;

	ComputePipeline m_fxaaPipeline;

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

	int m_wrapSampler;
	int m_clampSampler;

	int m_viewProjBuffer;

	int m_shadowVPBuffer;

	int m_screenInfoBuffer;

	std::unique_ptr<Camera> m_camera;
	std::unique_ptr<ShadowCamera> m_shadowcamera;
	std::unique_ptr<CubeCamera> m_cubecamera;

	int m_frame = 0;

	static const int CUBE_MAP_SIZE = 256;
	static const int SHADOW_MAP_SIZE = 2048;
};

#endif