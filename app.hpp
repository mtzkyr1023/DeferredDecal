
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

#include "render_pass/geometory_pass.h"
#include "render_pass/last_pass.h"

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

private:
	Device m_device;
	Queue m_queue;
	Swapchain m_swapchain;
	std::vector<CommandAllocator> m_commandAllocator;
	std::vector<CommandList> m_commandList;
	GeometoryPass m_geoPass;
	LastPass m_lastPass;
	Fence m_fence;

	MyGui m_gui;

	static const UINT m_width = 1280;
	static const UINT m_height = 720;
	static const UINT backBufferCount = 2;
	static const UINT portalCount = 2;
};