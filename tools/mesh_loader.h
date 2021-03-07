#ifndef _MESHLOADER_H_
#define _MESHLOADER_H_

#include <vector>
#include "../glm-master/glm/glm.hpp"
#include "../glm-master/glm/gtc/matrix_transform.hpp"
#include "../glm-master/glm/gtc/type_ptr.hpp"


class MeshLoader {
public:
	MeshLoader() = default;
	~MeshLoader() = default;


	bool loadMeshFromGltf(const char* filename, bool isBinary = false);
	bool loadMeshFromPmx(const char* filename);

	void destroy();

	std::vector<glm::vec3>& getPos(int num) { return m_pos[num]; }
	std::vector<glm::vec3>& getNor(int num) { return m_nor[num]; }
	std::vector<glm::vec2>& getTex1(int num) { return m_tex1[num]; }
	std::vector<glm::vec3>& getTan(int num) { return m_tan[num]; }
	std::vector<glm::vec2>& getTex2(int num) { return m_tex2[num]; }
	std::vector<glm::vec4>& getBoneWeight(int num) { return m_weight[num]; }
	std::vector<glm::uvec4>& getBoneIndex(int num) { return m_index[num]; }

	uint32_t getMaterialCount() { return m_materialCount; }

	uint32_t getAlbedoImageIndex(int num) { return m_albedoImageIndex[num]; }
	uint32_t getNormalImageIndex(int num) { return m_normalImageIndex[num]; }
	uint32_t getOccrusionImageIndex(int num) { return m_occrusionImageIndex[num]; }
	uint32_t getPbrImageIndex(int num) { return m_pbrImageIndex[num]; }

	uint32_t getImageCount() { return (uint32_t)m_imageData.size(); }
	std::vector<unsigned char>& getImageData(int num) { return m_imageData[num]; }
	uint32_t getImageWidth(int num) { return m_imageWidth[num]; }
	uint32_t getImageHeight(int num) { return m_imageHeight[num]; }

	std::vector<uint32_t>& getIndexArray(int num) { return m_indexArray[num]; }

	uint32_t getAllVertexCount() { return m_allVertexCount; }
	uint32_t getAllIndexCount() { return m_allIndexCount; }

	std::vector<uint32_t>& getVertexCount() { return m_vertexCount; }
	std::vector<uint32_t>& getIndexCount() { return m_indexCount; }

private:
	std::vector<std::vector<glm::vec3>> m_pos;
	std::vector<std::vector<glm::vec3>> m_nor;
	std::vector<std::vector<glm::vec2>> m_tex1;
	std::vector<std::vector<glm::vec3>> m_tan;
	std::vector<std::vector<glm::vec2>> m_tex2;
	std::vector<std::vector<glm::vec4>> m_weight;
	std::vector<std::vector<glm::uvec4>> m_index;

	std::vector<std::vector<uint32_t>> m_indexArray;

	std::vector<uint32_t> m_albedoImageIndex;
	std::vector<uint32_t> m_normalImageIndex;
	std::vector<uint32_t> m_occrusionImageIndex;
	std::vector<uint32_t> m_pbrImageIndex;

	std::vector<std::vector<unsigned char>> m_imageData;
	std::vector<uint32_t> m_imageWidth;
	std::vector<uint32_t> m_imageHeight;

	uint32_t m_materialCount;
	uint32_t m_allIndexCount;
	uint32_t m_allVertexCount;

	std::vector<uint32_t> m_vertexCount;
	std::vector<uint32_t> m_indexCount;
};

#endif