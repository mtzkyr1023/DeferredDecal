#ifndef _MODEL_H_
#define _MODEL_H_

#include "../framework/buffer.h"
#include "../framework/texture.h"

#include <unordered_map>
#include <memory>

#include "../glm-master/glm/glm.hpp"
#include "../glm-master/glm/gtc/matrix_transform.hpp"
#include "../glm-master/glm/gtc/quaternion.hpp"


#define DEFAULT_MESH "models/cube.gltf"


class Mesh {
public:
	Mesh();
	~Mesh();

	bool createFromGltf(ID3D12Device* dev, ID3D12CommandQueue* queue,
		uint32_t bufferCount, const char* filename, bool binary = false);

	VertexBuffer* getVertexBuffer() { return &m_vertexBuffer; }
	IndexBuffer* getIndexBuffer() { return &m_indexBuffer; }
	StructuredBuffer<glm::vec4>* getUVBuffer() { return &m_uvBuffer; }

	uint32_t getAllIndexCount() { return m_allIndexCount; }

	uint32_t getMaterialCount() { return m_materialCount; }

	uint32_t getVertexCount(uint32_t num) { return m_vertexCount[num]; }
	uint32_t getIndexCount(uint32_t num) { return m_indexCount[num]; }

	Texture* getAlbedoTexture(uint32_t num) {
		if (m_albedoImageIndex[num] == 0xffffffff)
			return nullptr;
		else
			return &m_images[m_albedoImageIndex[num]];
	}
	Texture* getNormalTexture(uint32_t num) {
		if (m_normalImageIndex[num] == 0xffffffff)
			return nullptr;
		else
			return &m_images[m_normalImageIndex[num]];
	}
	Texture* getRoughMetalTexture(uint32_t num) {
		if (m_roughMetalImageIndex[num] == 0xffffffff)
			return nullptr;
		else
			return &m_images[m_roughMetalImageIndex[num]];
	}

	Texture* getTexture(uint32_t num) { return &m_images[num]; }

	uint32_t getTextureCount() { return m_images.size(); }
	uint32_t getAlbedoTextureINdex(uint32_t num) { return m_albedoImageIndex[num]; }
	uint32_t getNormalTextureINdex(uint32_t num) { return m_normalImageIndex[num]; }
	uint32_t getRoughMetalTextureINdex(uint32_t num) { return m_roughMetalImageIndex[num]; }

	glm::vec3& getMin() { return m_min; }
	glm::vec3& getMax() { return m_max; }

	std::vector<glm::vec3>& getPosArray() { return m_posArray; }
	std::vector<uint32_t>& getIndexArray() { return m_indexArray; }

	bool calculateUV(ID3D12Device* dev);

private:
	bool loadModelCache(ID3D12Device* device, ID3D12CommandQueue* queue, uint32_t bufferCount);
	void saveModelCache();

private:
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 nor;
		glm::vec3 tan;
		glm::vec2 tex;
	};


private:
	VertexBuffer m_vertexBuffer;
	IndexBuffer m_indexBuffer;

	StructuredBuffer<glm::vec4> m_uvBuffer;

	std::string m_filename;

	std::vector<Texture> m_images;
	std::vector<uint32_t> m_albedoImageIndex;
	std::vector<uint32_t> m_normalImageIndex;
	std::vector<uint32_t> m_roughMetalImageIndex;

	uint32_t m_materialCount;
	uint32_t m_imageCount;

	std::vector<std::vector<unsigned char>> m_imageDatas;
	std::vector<uint32_t> m_imageWidths;
	std::vector<uint32_t> m_imageHeights;
	std::vector<Vertex> m_vertexArray;

	std::vector<uint32_t> m_vertexCount;
	std::vector<uint32_t> m_indexCount;
	uint32_t m_allVertexCount;
	uint32_t m_allIndexCount;

	glm::vec3 m_min;
	glm::vec3 m_max;

	std::vector<glm::vec3> m_posArray;
	std::vector<glm::vec3> m_norArray;
	std::vector<uint32_t> m_indexArray;

	std::vector<glm::vec2> m_texcoord;
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

	Mesh* getMesh(const char* filename) {
		if (m_meshArray.find(filename) != m_meshArray.end())
			return m_meshArray[filename].get();

		return nullptr;
	}

private:
	std::unordered_map<std::string, std::unique_ptr<Mesh>> m_meshArray;
};


#endif