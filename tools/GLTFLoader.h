#ifndef _GLTFLOADER_H_
#define _GLTFLOADER_H_

#include <vector>

class GLTFLoader {
private:
	GLTFLoader() = default;
	~GLTFLoader() = default;

public:
	GLTFLoader(const GLTFLoader&) = delete;
	GLTFLoader& operator=(const GLTFLoader&) = delete;
	GLTFLoader(GLTFLoader&&) = delete;
	GLTFLoader&& operator=(GLTFLoader&&) = delete;

	static GLTFLoader& Instance() {
		static GLTFLoader inst;
		return inst;
	}

	bool LoadModel(const char* filename, bool skinned = false, bool binary = false);

	char* GetBinaryData() { return m_data.data(); }

	void ReleaseData() {
		std::vector<char>().swap(m_data);
		m_meshCount = 0;
		std::vector<glm::vec4>().swap(m_position);
		std::vector<glm::vec4>().swap(m_normal);
		std::vector<glm::vec4>().swap(m_uv);
		std::vector<glm::vec3>().swap(m_tangent);
		std::vector<uint32_t>().swap(m_index);
	}

	uint32_t GetMeshCount() { return m_meshCount; }
	std::vector<glm::vec4>& GetPosition() { return m_position; }
	std::vector<glm::vec4>& GetNormal() { return m_normal; }
	std::vector<glm::vec4>& GetUV() { return m_uv; }
	std::vector<glm::vec3>& GetTangent() { return m_tangent; }
	std::vector<uint32_t>& GetIndex() { return m_index; }

private:
	std::vector<char> m_data;
	uint32_t m_meshCount = 0;
	std::vector<glm::vec4> m_position;
	std::vector<glm::vec4> m_normal;
	std::vector<glm::vec4> m_uv;
	std::vector<glm::vec3> m_tangent;
	std::vector<uint32_t> m_index;
};

#endif