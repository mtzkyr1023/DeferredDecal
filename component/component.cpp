#include "component.h"

int CreateNextID() {
	static int i = 0;

	return i++;
}

GameObject::~GameObject() {

}

void GameObject::execute(float deltaTime) {
	for (auto& ite : m_components) {
		for (auto& itr : ite.second) {
			if (itr->enabled) itr->execute(deltaTime);
		}
	}
}


uint32_t Scheduler::addGameObject(LAYER layer, const char* tag) {
	m_objectArray[m_id] = std::make_unique<GameObject>();
	m_objectArray[m_id]->id = m_id;
	m_objectArray[m_id]->layer = layer;
	m_objectArray[m_id]->tag = tag;

	m_objectPerLayer[layer].push_back(m_objectArray[m_id].get());

	return m_id++;
}

void Scheduler::eraseObject(int id) {
	for (auto& ite : m_objectPerLayer[m_objectArray[id]->layer]) {
		if (ite->id == id) {
			std::swap(ite, m_objectPerLayer[m_objectArray[id]->layer].back());
			m_objectPerLayer[m_objectArray[id]->layer].pop_back();
			break;
		}
	}

	m_objectArray.erase(id);
}

void Scheduler::execute(float deltaTime) {
	for (auto& ite : m_objectArray) {
		ite.second->execute(deltaTime);
	}
}