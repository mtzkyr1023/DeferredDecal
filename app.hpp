
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


#include "render_pipeline.h"


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

	MyGui m_gui;

	RenderPipeline m_renderPipeline;
};