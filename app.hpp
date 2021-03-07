
#include <Windows.h>

#include "framework/device.h"
#include "framework/commandbuffer.h"
#include "framework/descriptor_heap.h"
#include "framework/pipeline.h"
#include "framework/queue.h"
#include "framework/root_signature.h"
#include "framework/shader.h"
#include "framework/buffer.h"
#include "framework/texture.h"
#include "framework/fence.h"

#include "tools/model.h"
#include "tools/my_gui.h"

#include "glm-master/glm/glm.hpp"
#include "glm-master/glm/gtc/matrix_transform.hpp"
#include "glm-master/glm/gtc/quaternion.hpp"

class App {
public:
	App();
	~App();

	bool initialize(HWND hwnd);

	void shutdown();

	void render();

	void run(UINT curImageCount);

private:
	struct CB0 {
		glm::mat4 world;
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 padding;
	};

	struct CB2 {
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 invViewProj;
		glm::vec4 screenParam;
		glm::vec4 viewPos;
		glm::vec4 padding1;
		glm::vec4 padding2;
	};

	struct SB0 {
		glm::mat4 world;
		glm::mat4 invWorld;
	};

private:
	Device m_device;
	Queue m_queue;
	Swapchain m_swapchain;
	DescriptorHeap m_geometoryCbvHeap;
	DescriptorHeap m_lastSrvHeap;
	DescriptorHeap m_lastSamplerHeap;
	DescriptorHeap m_geometoryRtvHeap;
	DescriptorHeap m_decalCbvHeap;
	DescriptorHeap m_decalRtvHeap;
	DescriptorHeap m_decalSamplerHeap;
	DescriptorHeap m_rtvHeap;
	DescriptorHeap m_dsvHeap;
	RootSignature m_geometoryRootSignature;
	RootSignature m_decalRootSignature;
	RootSignature m_aoRootSignature;
	RootSignature m_lastRootSignature;
	std::vector<CommandAllocator> m_commandAllocator;
	std::vector<CommandList> m_commandList;
	Pipeline m_geometoryPipeline;
	Pipeline m_aoPipeline;
	Pipeline m_decalPipeline;
	Pipeline m_lastPipeline;
	Shader m_geometoryVS;
	Shader m_geometoryPS;
	Shader m_decalVS;
	Shader m_decalPS;
	Shader m_screenVS;
	Shader m_aoPS;
	Shader m_lastPS;
	Texture m_backBuffer;
	Texture m_depthBuffer;
	Texture m_colorBuffer;
	Texture m_normalBuffer;
	Texture m_aoBuffer;
	Texture m_decalTexture;
	Mesh m_mesh;
	Mesh m_cubeMesh;
	ConstantBuffer<CB0> m_cb0;
	ConstantBuffer<CB2> m_cb2;
	StructuredBuffer<SB0> m_sb0;
	Fence m_fence;

	MyGui m_gui;

	UINT m_decalID = 0;
	UINT m_decalCount = 0;
	static const UINT m_width = 1280;
	static const UINT m_height = 720;
	static const UINT backBufferCount = 2;
	static const UINT decalCount = 16;
};