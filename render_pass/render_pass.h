#ifndef _RENDER_PASS_H_
#define _RENDER_PASS_H_

#include "../framework/device.h"
#include "../framework/queue.h"

class RenderPass {
public:
	virtual ~RenderPass() = default;
};

#endif