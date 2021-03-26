#ifndef _COMPONENT_H_
#define _COMPONENT_H_

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

enum class LAYER {
	LAYER0 = 0x00000001,
	LAYER1 = 0x00000002,
	LAYER2 = 0x00000004,
	LAYER3 = 0x00000008,
	LAYER4 = 0x00000010,
	LAYER5 = 0x00000020,
	LAYER6 = 0x00000040,
	LAYER7 = 0x00000080,
};

int CreateNextID();

template<class T>
int GetComponentID() {
	static int i = CreateNextID();
	return i;
}

class GameObject;

class Component {
public:
	virtual ~Component() = default;

	virtual void execute() = 0;

	bool enabled = false;

	GameObject* parent = nullptr;
};


class GameObject {
public:
	GameObject() = default;
	~GameObject();

	template<class T>
	T* addComponent() {
		T* component = new T;
		m_components[GetComponentID<T>()].push_back(std::make_unique<T>(component));
		component->parent = this;

		return component;
	}

	template<class T>
	void removeComponent(int id = 0) {
		std::swap(m_components[GetComponentID<T>()][id], m_components[GetComponentID<T>()].back());
		m_components[GetComponentID<T>()].pop_back();
	}

	template<class T>
	T* getComponent(int id = 0) {
		if((uint32_t)id < m_components[GetComponentID<T>()].size())
			return static_cast<T*>(m_components[GetComponentID<T>()][id].get());

		return nullptr;
	}

	void execute();

	int id = 0;
	LAYER layer = LAYER::LAYER0;
	std::string name = "GameObject";
	std::string tag = "Default";

private:
	std::unordered_map<int, std::vector<std::unique_ptr<Component>>> m_components;
};

class Scheduler {
private:
	Scheduler() = default;
	~Scheduler() = default;

public:
	uint32_t addGameObject(LAYER layer, const char* tag);
	void eraseObject(int id);

	GameObject* getGameObject(int id) { return m_objectArray[id].get(); }

	std::vector<GameObject*>* getObjectPerLayer(LAYER layer) { return &m_objectPerLayer[layer]; }

	void execute();

	static Scheduler& instance() {
		static Scheduler inst;
		return inst;
	}

private:
	uint32_t m_id = 0;
	std::unordered_map<uint32_t, std::unique_ptr<GameObject>> m_objectArray;
	std::unordered_map<LAYER, std::vector<GameObject*>> m_objectPerLayer;
};

#endif