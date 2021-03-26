
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

private:
	Device m_device;
	Queue m_queue;
	Swapchain m_swapchain;
	DescriptorHeap m_geometoryCbvHeap;
	DescriptorHeap m_geometorySamplerHeap;
	DescriptorHeap m_geometoryRtvHeap;
	DescriptorHeap m_geometoryDsvHeap;
	DescriptorHeap m_camCbvHeap;
	DescriptorHeap m_camSamplerHeap;
	DescriptorHeap m_camRtvHeap;
	DescriptorHeap m_camDsvHeap;
	DescriptorHeap m_portalCbvHeap;
	DescriptorHeap m_portalSamplerHeap;
	DescriptorHeap m_portalRtvHeap;
	DescriptorHeap m_portalDsvHeap;
	DescriptorHeap m_lastCbvHeap;
	DescriptorHeap m_lastSamplerHeap;
	DescriptorHeap m_lastRtvHeap;
	RootSignature m_geometoryRootSignature;
	RootSignature m_portalRootSignature;
	RootSignature m_lastRootSignature;
	std::vector<CommandAllocator> m_commandAllocator;
	std::vector<CommandList> m_commandList;
	Pipeline m_geometoryPipeline;
	Pipeline m_portalPipeline;
	Pipeline m_lastPipeline;
	Shader m_geometoryVS;
	Shader m_geometoryPS;
	Shader m_portalVS;
	Shader m_portalPS;
	Shader m_screenVS;
	Shader m_lastPS;
	Texture m_backBuffer;
	Texture m_depthBuffer;
	Texture m_colorBuffer;
	Texture m_portalColorBuffer;
	Texture m_portalDepthBuffer;
	Texture m_resultColorTexture;
	Mesh m_mesh;
	Mesh m_cubeMesh;
	ConstantBuffer<CB0> m_cb0;
	ConstantBuffer<CB0> m_portalCB0;
	ConstantBuffer<CB0> m_camCB0;
	Fence m_fence;

	MyGui m_gui;

	static const UINT m_width = 1280;
	static const UINT m_height = 720;
	static const UINT backBufferCount = 2;
	static const UINT portalCount = 2;
};