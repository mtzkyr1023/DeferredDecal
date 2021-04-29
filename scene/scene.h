#ifndef _SCENEN_H_
#define _SCENEN_H_

#include "../component/component.h"

class Scene {
public:
	Scene() = default;
	~Scene() = default;

	virtual void init() {}

	virtual void run() {}

	std::vector<GameObject> gameObjects;
};


#endif