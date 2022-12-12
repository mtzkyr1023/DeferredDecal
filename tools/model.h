#ifndef _MODEL_H_
#define _MODEL_H_

#include "../framework/buffer.h"
#include "../framework/texture.h"
#include "../framework/commandbuffer.h"

#include <unordered_map>
#include <memory>

#include "../glm-master/glm/glm.hpp"
#include "../glm-master/glm/gtc/matrix_transform.hpp"
#include "../glm-master/glm/gtc/quaternion.hpp"


#define DEFAULT_MESH "models/cube.gltf"	


class Model {
public:

	struct Instance {
		glm::vec3 aabbmin;
		glm::vec3 aabbmax;
		uint32_t indexOffset;
		uint32_t indexCount;
		uint64_t materialId;
	};

	struct Vertex {
		glm::vec4 pos;
		glm::vec4 nor;
		glm::vec4 tan;
		glm::vec4 tex;

		Vertex() {
			this->pos = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
			this->nor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
			this->tan = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
			this->tex = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		}
		Vertex(const glm::vec3& pos, const glm::vec3& nor, const glm::vec3& tan, glm::vec2& tex) {
			this->pos = glm::vec4(pos, 0.0f);
			this->nor = glm::vec4(nor, 0.0f);;
			this->tan = glm::vec4(tan, 0.0f);
			this->tex = glm::vec4(tex, 0.0f, 0.0f);
		}
	};

	Model();
	~Model();

	bool createFromGltf(ID3D12Device* dev, ID3D12CommandQueue* queue,
		uint32_t bufferCount, const char* filename, bool binary = false);

	int getVertexBuffer() { return m_vertexBuffer; }
	int getIndexBuffer() { return m_indexBuffer; }
	int getInstanceBuffer() { return m_instanceBuffer; }

	uint32_t getAllVertexCount() { return m_allVertexCount; }
	uint32_t getAllIndexCount() { return m_allIndexCount; }

	uint32_t getMaterialCount() { return m_materialCount; }

	uint32_t getVertexCount(uint32_t num) { return m_vertexCount[num]; }
	uint32_t getIndexCount(uint32_t num) { return m_indexCount[num]; }

	int getAlbedoTexture(uint32_t num) {
		if(m_images.size() <= num)
			return m_defaultImage;
		if (m_albedoImageIndex[num] == 0xffffffff)
			return m_defaultImage;
		else
			return m_images[m_albedoImageIndex[num]];
	}
	int getNormalTexture(uint32_t num) {
		if (m_images.size() <= num)
			return m_defaultImage;
		if (m_normalImageIndex[num] == 0xffffffff)
			return m_defaultImage;
		else
			return m_images[m_normalImageIndex[num]];
	}
	int getRoughMetalTexture(uint32_t num) {
		if (m_images.size() <= num)
			return m_defaultImage;
		if (m_roughMetalImageIndex[num] == 0xffffffff)
			return m_defaultImage;
		else
			return m_images[m_roughMetalImageIndex[num]];
	}

	int getTexture(uint32_t num) { return m_images[num]; }

	uint32_t getTextureCount() { return (uint32_t)m_images.size(); }
	uint32_t getAlbedoTextureINdex(uint32_t num) { return m_albedoImageIndex[num]; }
	uint32_t getNormalTextureINdex(uint32_t num) { return m_normalImageIndex[num]; }
	uint32_t getRoughMetalTextureINdex(uint32_t num) { return m_roughMetalImageIndex[num]; }

	glm::vec3& getMin() { return m_min; }
	glm::vec3& getMax() { return m_max; }

	std::vector<glm::vec4>& getPosArray() { return m_positionArray; }
	std::vector<uint32_t>& getIndexArray() { return m_indexArray; }

	bool calculateUV(ID3D12Device* dev, ID3D12CommandQueue* queue);

	uint32_t getInstanceCount() { return (uint32_t)m_instanceArray.size(); }

	ID3D12GraphicsCommandList* getBundle() { return m_bundle.getCommandList(); }

	void createBundle(ID3D12Device* device, ID3D12RootSignature* rootSignature);

	int instanceToIndexMap() { return m_instanceToIndexMap; }

private:
	bool loadModelCache(ID3D12Device* device, ID3D12CommandQueue* queue, uint32_t bufferCount);
	void saveModelCache();


private:
	int m_vertexBuffer;

	int m_weightBuffer;
	int m_boneIndexBuffer;

	int m_indexBuffer;

	int m_instanceBuffer;

	int m_instanceToIndexMap;

	std::string m_filename;

	int m_defaultImage;
	std::vector<int> m_images;
	std::vector<uint32_t> m_albedoImageIndex;
	std::vector<uint32_t> m_normalImageIndex;
	std::vector<uint32_t> m_roughMetalImageIndex;

	uint32_t m_materialCount;
	uint32_t m_imageCount;

	std::vector<std::vector<unsigned char>> m_imageDatas;
	std::vector<uint32_t> m_imageWidths;
	std::vector<uint32_t> m_imageHeights;
	std::vector<glm::vec4> m_positionArray;
	std::vector<glm::vec4> m_normalArray;
	std::vector<glm::vec4> m_tangentArray;
	std::vector<glm::vec4> m_uvArray;
	std::vector<Vertex> m_vertexArray;

	std::vector<glm::vec2> m_texcoord;

	std::vector<uint32_t> m_vertexCount;
	std::vector<uint32_t> m_indexCount;
	uint32_t m_allVertexCount;
	uint32_t m_allIndexCount;

	glm::vec3 m_min;
	glm::vec3 m_max;

	std::vector<uint32_t> m_indexArray;

	std::vector<Instance> m_instanceArray;
	std::vector<int> m_materialIdBuffer;

	CommandList m_bundle;
};
	

class MeshManager {
private:
	MeshManager() = default;
	~MeshManager() = default;

public:
	static MeshManager& instance() {
		static MeshManager inst;
		return inst;
	}

	bool createMeshFromGltf(ID3D12Device* dev, ID3D12CommandQueue* queue,
		uint32_t bufferCount, const char* filename, bool binary = false);

	void releaseMesh(const char* filename);
	void allRelease();

	Model* getModel(const char* filename) {
		if (m_meshArray.find(filename) != m_meshArray.end())
			return m_meshArray[filename].get();

		return nullptr;
	}

private:
	std::unordered_map<std::string, std::unique_ptr<Model>> m_meshArray;
};


#endif