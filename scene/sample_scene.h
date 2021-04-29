#ifndef _SAMPLE_SCENE_H_
#define _SAMPLE_SCENE_H_

#include "scene.h"

class SampleScene : public Scene {
public:
	SampleScene() = default;
	~SampleScene() = default;

	void init();

	void run();
};

#endif