#ifndef _OCT_TREE_H_
#define _OCT_TREE_H_

#include <memory>
#include <vector>
#include <list>
#include <map>


template<class T>
class Cell;

template<class T>
class ObjectForTree {
public:
	Cell<T>* cell;
	T* object;
	std::shared_ptr<ObjectForTree<T>> pre;
	std::shared_ptr<ObjectForTree<T>> next;

public:
	ObjectForTree() {
		cell = nullptr;
	}

	virtual ~ObjectForTree() {

	}

public:
	bool remove() {
		if (!cell)
			return false;

		if (!cell->onRemove(this))
			return false;

		if (pre) {
			pre->next = next;
			pre = nullptr;
		}
		if (next) {
			next->pre = pre;
			next = nullptr;
		}

		cell = nullptr;

		return true;
	}

	void registCell(Cell<T>* pCell) {
		cell = pCell;
	}

	std::shared_ptr<ObjectForTree<T>>& getNextObj() {
		return next;
	}
};

#define LINER8TREEMANAGER_MAXLEVEL 7

template<class T>
class Linear8TreeManager {
protected:
	unsigned int m_dim;
	Cell<T>** m_cellArray;
	unsigned int m_pow[LINER8TREEMANAGER_MAXLEVEL + 1];
	float m_w;
	float m_h;
	float m_d;
	float m_minW;
	float m_minH;
	float m_minD;
	float m_maxW;
	float m_maxH;
	float m_maxD;
	float m_unitW;
	float m_unitH;
	float m_unitD;
	unsigned long m_cellNum;
	unsigned int m_level;

public:
	Linear8TreeManager() {
		m_level = 0;
		m_w = 0.0f;
		m_h = 0.0f;
		m_d = 0.0f;
		m_minW = 0.0f;
		m_minH = 0.0f;
		m_minD = 0.0f;
		m_maxW = 0.0f;
		m_maxH = 0.0f;
		m_maxD = 0.0f;
		m_unitW = 0.0f;
		m_unitH = 0.0f;
		m_unitD = 0.0f;
		m_cellNum = 0;
		m_cellArray = nullptr;
		m_dim = 0;
	}

	virtual ~Linear8TreeManager() {
		for (unsigned long i = 0; i < m_cellNum; i++) {
			if (m_cellArray[i] != nullptr) {
				delete m_cellArray[i];
				m_cellArray[i] = nullptr;
			}
		}
		delete[] m_cellArray;
	}

	bool init(unsigned int level, float minW, float maxW, float minH, float maxH, float minD, float maxD) {
		if (level >= LINER8TREEMANAGER_MAXLEVEL)
			return false;

		m_pow[0] = 1;
		for (int i = 1; i < LINER8TREEMANAGER_MAXLEVEL + 1; i++)
			m_pow[i] = m_pow[i - 1] * 8;

		m_cellNum = (m_pow[level + 1] - 1) / 7;

		m_cellArray = new Cell<T>*[m_cellNum];
		memset(m_cellArray, 0, sizeof(Cell<T>*) * m_cellNum);

		m_minW = minW;
		m_minH = minH;
		m_minD = minD;
		m_maxW = maxW;
		m_maxH = maxH;
		m_maxD = maxD;
		m_w = maxW - minW;
		m_h = maxH - minH;
		m_d = maxD - minD;
		m_unitW = m_w / (float)(1 << level);
		m_unitH = m_h / (float)(1 << level);
		m_unitD = m_d / (float)(1 << level);

		m_level = level;

		return true;
	}

	bool regist(float min_x, float max_x, float min_y, float max_y, float min_z, float max_z, std::shared_ptr<ObjectForTree<T>>& oft) {
		unsigned long elem = getMortonNumber(min_x, max_x, min_y, max_y, min_z, max_z);
		if (elem < m_cellNum) {
			if (m_cellArray[elem] == nullptr)
				createNewCell(elem);
			return m_cellArray[elem]->push(oft);
		}
		return false;
	}

	unsigned long getAllCollisionList(std::vector<T*>& colArray) {
		colArray.clear();

		if (m_cellArray[0] == nullptr)
			return 0;

		std::list<T*> colStac;
		getCollisionList(0, colArray, colStac);

		return (unsigned long)colArray.size();
	}

protected:
	bool getCollisionList(unsigned long elem, std::vector<T*>& colArray, std::list<T*>& colStac) {

		std::shared_ptr<ObjectForTree<T>> oft1 = m_cellArray[elem]->getFirstObj();
		while (oft1) {
			std::shared_ptr<ObjectForTree<T>> oft2 = oft1->next;
			while (oft2) {
				colArray.push_back(oft1->object);
				colArray.push_back(oft2->object);
				oft2 = oft2->next;
			}

			for (auto it = colStac.begin(); it != colStac.end(); it++) {
				colArray.push_back(oft1->object);
				colArray.push_back(*it);
			}
			oft1 = oft1->next;
		}

		bool childFlag = false;
		unsigned long objNum = 0;
		unsigned long i, nextElem;
		for (i = 0; i < 4; i++) {
			nextElem = elem * 4 + 1 + i;
			if (nextElem < m_cellNum && m_cellArray[elem * 4 + 1 + i]) {
				if (!childFlag) {
					oft1 = m_cellArray[elem]->getFirstObj();

					while (oft1) {
						colStac.push_back(oft1->object);
						objNum++;
						oft1 = oft1->next;
					}
				}
				childFlag = true;
				getCollisionList(elem * 4 + 1 + i, colArray, colStac);
			}
		}

		if (childFlag) {
			for (i = 0; i < objNum; i++)
				colStac.pop_back();
		}

		return true;
	}

	bool createNewCell(unsigned long elem) {
		while (!m_cellArray[elem]) {
			m_cellArray[elem] = new Cell<T>;

			elem = (elem - 1) >> 3;
			if (elem >= m_cellNum) break;
		}

		return true;
	}

	unsigned long getMortonNumber(float min_x, float max_x, float min_y, float max_y, float min_z, float max_z) {
		unsigned long LT = getPointElem(min_x, min_y, min_z);
		unsigned long RB = getPointElem(max_x, max_y, max_z);

		unsigned long def = RB ^ LT;
		unsigned int hiLevel = 1;
		unsigned int i;
		for (i = 0; i < m_level; i++) {
			unsigned long check = (def >> (i * 3)) & 0x7;
			if (check != 0)
				hiLevel = i + 1;
		}
		unsigned long spaceNum = RB >> (hiLevel * 3);
		unsigned long addNum = (m_pow[m_level - hiLevel] - 1) / 7;
		spaceNum += addNum;

		if (spaceNum > m_cellNum)
			return 0xffffffff;

		return spaceNum;
	}

	unsigned long bitSeparateFor3D(unsigned char n)
	{
		unsigned long s = n;
		s = (s | s << 8) & 0x0000f00f;
		s = (s | s << 4) & 0x000c30c3;
		s = (s | s << 2) & 0x00249249;
		return s;
	}

	unsigned long get3DMortonNumber(unsigned char x, unsigned char y, unsigned char z)
	{
		return (bitSeparateFor3D(x) | (bitSeparateFor3D(y) << 1) | (bitSeparateFor3D(z) << 2));
	}

	unsigned long getPointElem(float pos_x, float pos_y, float pos_z)
	{
		return get3DMortonNumber(
			(unsigned char)((pos_x - m_minW) / m_unitW),
			(unsigned char)((pos_y - m_minH) / m_unitH),
			(unsigned char)((pos_z - m_minD) / m_unitD)
		);
	}
};


template<class T>
class Cell {
protected:
	std::shared_ptr<ObjectForTree<T>> m_latest;

public:
	Cell() {

	}

	virtual ~Cell() {
		if (m_latest)
			resetLink(m_latest);
	}

	void resetLink(std::shared_ptr<ObjectForTree<T>>& oft) {
		if (oft->next)
			resetLink(oft->next);
		oft.reset();
	}

	bool push(std::shared_ptr<ObjectForTree<T>>& oft) {
		if (!oft) return false;
		if (oft->cell == this) return false;
		if (!m_latest) {
			m_latest = oft;
		}
		else {
			oft->next = m_latest;
			m_latest->pre = oft;
			m_latest = oft;
		}
		oft->registCell(this);
		return true;
	}

	std::shared_ptr<ObjectForTree<T>> getFirstObj() {
		return m_latest;
	}

	bool onRemove(ObjectForTree<T>* removeObj) {
		if (m_latest.get() == removeObj) {
			if (m_latest)
				m_latest = m_latest->getNextObj();
		}
		return true;
	}
};

#endif