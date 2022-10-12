#define TINYGLTF_IMPLEMENTATION

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include <unordered_map>
using namespace std;

#include "mesh_loader.h"

#include "../tools/tiny_gltf.h"

using namespace std;


bool MeshLoader::loadMeshFromGltf(const char* filename, bool isBinary) {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	m_allVertexCount = 0;
	m_allIndexCount = 0;

	string err;
	string warn;

	bool ret;

	if (!isBinary)
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	else
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);

	if (!ret)
		return false;

	uint32_t materialIndex;
	uint32_t meshCount = (uint32_t)model.meshes[0].primitives.size();
	m_materialCount = (uint32_t)model.materials.size();
	vector<vector<glm::vec3>> tempPos(model.meshes[0].primitives.size());
	vector<vector<glm::vec3>> tempNor(model.meshes[0].primitives.size());
	vector<vector<glm::vec2>> tempTex1(model.meshes[0].primitives.size());
	vector<vector<glm::vec2>> tempTex2(model.meshes[0].primitives.size());
	vector<vector<glm::vec3>> tempTan(model.meshes[0].primitives.size());
	vector<vector<glm::vec4>> tempWeight(model.meshes[0].primitives.size());
	vector<vector<glm::uvec4>> tempIndex(model.meshes[0].primitives.size());
	vector<vector<glm::uint32_t>> tempIndexArray(model.meshes[0].primitives.size());

	m_pos.resize(m_materialCount);
	m_nor.resize(m_materialCount);
	m_tex1.resize(m_materialCount);
	m_tan.resize(m_materialCount);
	m_tex2.resize(m_materialCount);
	m_weight.resize(m_materialCount);
	m_index.resize(m_materialCount);
	m_indexArray.resize(m_materialCount);
	m_indexCount.resize(m_materialCount);
	m_vertexCount.resize(m_materialCount);

	unordered_map<uint32_t, uint32_t> materialMemory;

	for (uint32_t i = 0; i < model.meshes.size(); i++) {
		const tinygltf::Mesh& mesh = model.meshes[i];
		for (size_t j = 0; j < mesh.primitives.size(); j++) {
			const tinygltf::Primitive& primitive = mesh.primitives[j];
			const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
			const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
			const tinygltf::Buffer& index = model.buffers[indexView.buffer];

			materialIndex = (uint32_t)primitive.material;

			materialMemory[(uint32_t)j] = (uint32_t)materialIndex;

			for (auto& ite : primitive.attributes) {
				const tinygltf::Accessor& accessor = model.accessors[ite.second];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

				size_t offset = (max)(accessor.byteOffset, bufferView.byteOffset);
				offset = accessor.byteOffset + bufferView.byteOffset;


				if (ite.first.compare("NORMAL") == 0) {
					glm::vec3 nor;
					if (tempNor.size() <= j) {
						tempNor.resize(j + 1);
					}
					for (size_t k = 0; k < accessor.count; k++) {
						memcpy_s(&nor, sizeof(glm::vec3), offset + buffer.data.data() + k * sizeof(glm::vec3), sizeof(glm::vec3));
						//						cout << "NOR:" << nor.x << "," << nor.y << "," << nor.z << endl;;
						tempNor[j].push_back(nor);
					}
				}
				else if (ite.first.compare("POSITION") == 0) {
					glm::vec3 pos;
					if (tempPos.size() <= j) {
						tempPos.resize(j + 1);
					}
					for (size_t k = 0; k < accessor.count; k++) {
						memcpy_s(&pos, sizeof(glm::vec3), offset + buffer.data.data() + k * sizeof(glm::vec3), sizeof(glm::vec3));
						//						cout << "POS:" << pos.x << "," << pos.y << "," << pos.z << endl;;
						tempPos[j].push_back(pos);
					}
				}
				else if (ite.first.compare("TEXCOORD_0") == 0) {
					glm::vec2 tex;
					if (tempTex1.size() <= j) {
						tempTex1.resize(j + 1);
					}
					for (size_t k = 0; k < accessor.count; k++) {
						memcpy_s(&tex, sizeof(glm::vec2), offset + buffer.data.data() + k * sizeof(glm::vec2), sizeof(glm::vec2));
						//						cout << "TEX:" << tex.x << "," << tex.y << endl;
						tempTex1[j].push_back(tex);
					}
				}
				else if (ite.first.compare("TEXCOORD_1") == 0) {
					glm::vec2 tex;
					if (tempTex2.size() <= j) {
						tempTex2.resize(j + 1);
					}
					for (size_t k = 0; k < accessor.count; k++) {
						memcpy_s(&tex, sizeof(glm::vec2), offset + buffer.data.data() + k * sizeof(glm::vec2), sizeof(glm::vec2));
						//						cout << "TEX:" << tex.x << "," << tex.y << endl;
						tempTex2[j].push_back(tex);
					}
				}
				else if (ite.first.compare("TANGENT") == 0) {
					glm::vec3 tan;
					if (tempTan.size() <= j) {
						tempTan.resize(j + 1);
					}
					for (size_t k = 0; k < accessor.count; k++) {
						memcpy_s(&tan, sizeof(glm::vec3), offset + buffer.data.data() + k * sizeof(glm::vec3), sizeof(glm::vec3));
						//						cout << "TAN:" << tan.x << "," << tan.y << "," << tan.z << endl;
						tempTan[j].push_back(tan);
					}
				}
				else if (ite.first.compare("WEIGHTS_0") == 0) {
					glm::vec4 weight;
					if (tempWeight.size() <= j) {
						tempWeight.resize(j + 1);
					}
					for (size_t k = 0; k < accessor.count; k++) {
						memcpy_s(&weight, sizeof(glm::vec4), offset + buffer.data.data() + k * sizeof(glm::vec4), sizeof(glm::vec4));
						tempWeight[j].push_back(weight);
					}
				}
				else if (ite.first.compare("JOINTS_0") == 0) {
					glm::ivec4 boneIndex;
					if (tempIndex.size() <= j) {
						tempIndex.resize(j + 1);
					}
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
						BYTE tmpIndex[4];
						for (size_t k = 0; k < accessor.count; k++) {
							memcpy_s(tmpIndex, sizeof(BYTE) * 4, offset + buffer.data.data() + k * sizeof(BYTE) * 4, sizeof(BYTE) * 4);
							boneIndex.x = (UINT)tmpIndex[0];
							boneIndex.y = (UINT)tmpIndex[1];
							boneIndex.z = (UINT)tmpIndex[2];
							boneIndex.w = (UINT)tmpIndex[3];
							tempIndex[j].push_back(boneIndex);
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
						USHORT tmpIndex[4];
						for (size_t k = 0; k < accessor.count; k++) {
							memcpy_s(tmpIndex, sizeof(USHORT) * 4, offset + buffer.data.data() + k * sizeof(USHORT) * 4, sizeof(USHORT) * 4);
							boneIndex.x = (UINT)tmpIndex[0];
							boneIndex.y = (UINT)tmpIndex[1];
							boneIndex.z = (UINT)tmpIndex[2];
							boneIndex.w = (UINT)tmpIndex[3];
							tempIndex[j].push_back(boneIndex);
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
						UINT tmpIndex[4];
						for (size_t k = 0; k < accessor.count; k++) {
							memcpy_s(tmpIndex, sizeof(UINT) * 4, buffer.data.data() + k * sizeof(UINT) * 4, sizeof(UINT) * 4);
							boneIndex.x = (UINT)tmpIndex[0];
							boneIndex.y = (UINT)tmpIndex[1];
							boneIndex.z = (UINT)tmpIndex[2];
							boneIndex.w = (UINT)tmpIndex[3];
							tempIndex[j].push_back(boneIndex);
						}
					}
				}
			}

			if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
				BYTE tmp;
				if (tempIndexArray.size() <= j) {
					tempIndexArray.resize(j + 1);
				}
				for (size_t k = 0; k < indexAccessor.count; k++) {
					memcpy_s(&tmp, sizeof(BYTE), index.data.data() + indexAccessor.byteOffset + k * sizeof(BYTE), sizeof(BYTE));
					//					cout << tmp << endl;
					tempIndexArray[j].push_back((UINT)tmp);
				}
			}
			else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
				USHORT tmp;
				if (tempIndexArray.size() <= j) {
					tempIndexArray.resize(j + 1);
				}
				for (size_t k = 0; k < indexAccessor.count; k++) {
					memcpy_s(&tmp, sizeof(SHORT), index.data.data() + indexView.byteOffset + k * sizeof(SHORT), sizeof(SHORT));
					//					cout << tmp << endl;
					tempIndexArray[j].push_back((UINT)tmp);
				}
			}
			else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
				UINT tmp;
				if (tempIndexArray.size() <= j) {
					tempIndexArray.resize(j + 1);
				}
				for (size_t k = 0; k < indexAccessor.count; k++) {
					memcpy_s(&tmp, sizeof(UINT), index.data.data() + indexView.byteOffset + k * sizeof(UINT), sizeof(UINT));
					//					cout << tmp << endl;
					tempIndexArray[j].push_back(tmp);
				}
			}
		}
	}

	m_albedoImageIndex.resize(model.materials.size());
	m_normalImageIndex.resize(model.materials.size());
	m_occrusionImageIndex.resize(model.materials.size());
	m_pbrImageIndex.resize(model.materials.size());
	for (size_t i = 0; i < model.materials.size(); i++) {
		m_albedoImageIndex[i] = model.materials[i].pbrMetallicRoughness.baseColorTexture.index;
		m_normalImageIndex[i] = model.materials[i].normalTexture.index;
		m_occrusionImageIndex[i] = model.materials[i].occlusionTexture.index;
		m_pbrImageIndex[i] = model.materials[i].pbrMetallicRoughness.metallicRoughnessTexture.index;
	}

	m_imageData.resize(model.images.size());
	m_imageWidth.resize(model.images.size());
	m_imageHeight.resize(model.images.size());
	for (size_t i = 0; i < model.images.size(); i++) {
		m_imageWidth[i] = (uint32_t)model.images[i].width;
		m_imageHeight[i] = (uint32_t)model.images[i].height;
		m_imageData[i].resize(m_imageWidth[i] * m_imageHeight[i] * (model.images[i].bits / 8 * model.images[i].component));
		m_imageData[i] = model.images[i].image;
	}

	for (uint32_t i = 0; i < meshCount; i++) {
		UINT vertexCount = (UINT)tempPos[i].size();
		vertexCount = max(vertexCount, (UINT)tempNor[i].size());
		vertexCount = max(vertexCount, (UINT)tempTex1[i].size());
		vertexCount = max(vertexCount, (UINT)tempTan[i].size());
		vertexCount = max(vertexCount, (UINT)tempTex2[i].size());

		if (tempPos[i].size() < vertexCount) {
			UINT tmpSize = (UINT)tempPos[i].size();
			tempPos[i].resize(vertexCount);
			for (UINT j = tmpSize; j < vertexCount; j++) {
				tempPos[i][j].x = tempPos[i][j].y = tempPos[i][j].z = 0.1f;
			}
		}
		if (tempNor[i].size() < vertexCount) {
			UINT tmpSize = (UINT)tempNor[i].size();
			tempNor[i].resize(vertexCount);
			for (UINT j = tmpSize; j < vertexCount; j++) {
				tempNor[i][j].x = tempNor[i][j].y = tempNor[i][j].z = 0.0f;
			}
		}
		if (tempTan[i].size() < vertexCount) {
			UINT tmpSize = (UINT)tempTan[i].size();
			tempTan[i].resize(vertexCount);
			for (UINT j = tmpSize; j < vertexCount; j++) {
				tempTan[i][j].x = tempTan[i][j].y = tempTan[i][j].z = 0.0f;
			}
		}
		if (tempTex1[i].size() < vertexCount) {
			UINT tmpSize = (UINT)tempTex1[i].size();
			tempTex1[i].resize(vertexCount);
			for (UINT j = tmpSize; j < vertexCount; j++) {
				tempTex1[i][j].x = tempTex1[i][j].y = 1.0f;
			}
		}
		if (tempTex2[i].size() < vertexCount) {
			UINT tmpSize = (UINT)tempTex2[i].size();
			tempTex2[i].resize(vertexCount);
			for (UINT j = tmpSize; j < vertexCount; j++) {
				tempTex2[i][j].x = tempTex2[i][j].y = 1.0f;
			}
		}
		if (tempTan[i].size() < vertexCount) {
			UINT tmpSize = (UINT)tempTan[i].size();
			tempTan[i].resize(vertexCount);
			for (UINT j = 0; j < (UINT)tempTan[i].size(); j++) {
				tempTan[i][j] = glm::vec3(0.0f);
			}
			for (UINT j = 0; j < m_indexArray[i].size(); j += 3) {
				UINT index0 = tempIndexArray[i][j + 0];
				UINT index1 = tempIndexArray[i][j + 1];
				UINT index2 = tempIndexArray[i][j + 2];
				if (index0 >= (UINT)tempPos[i].size() || index1 >= (UINT)tempPos[i].size() || index2 >= (UINT)tempPos[i].size())
					continue;

				glm::vec3 cp0[3] = {
					glm::vec3(tempPos[i][index0].x, tempTex1[i][index0].x, tempTex1[i][index0].y),
					glm::vec3(tempPos[i][index0].y, tempTex1[i][index0].x, tempTex1[i][index0].y),
					glm::vec3(tempPos[i][index0].z, tempTex1[i][index0].x, tempTex1[i][index0].y),
				};
				glm::vec3 cp1[3] = {
					glm::vec3(tempPos[i][index1].x, tempTex1[i][index1].x, tempTex1[i][index1].y),
					glm::vec3(tempPos[i][index1].y, tempTex1[i][index1].x, tempTex1[i][index1].y),
					glm::vec3(tempPos[i][index1].z, tempTex1[i][index1].x, tempTex1[i][index1].y),
				};
				glm::vec3 cp2[3] = {
					glm::vec3(tempPos[i][index2].x, tempTex1[i][index2].x, tempTex1[i][index2].y),
					glm::vec3(tempPos[i][index2].y, tempTex1[i][index2].x, tempTex1[i][index2].y),
					glm::vec3(tempPos[i][index2].z, tempTex1[i][index2].x, tempTex1[i][index2].y),
				};

				for (int k = 0; k < 3; k++) {
					glm::vec3 v1 = cp1[k] - cp0[k];
					glm::vec3 v2 = cp2[k] - cp1[k];

					glm::vec3 ABC = glm::cross(v1, v2);

					tempTan[i][index0][k] = -ABC.y / ABC.x;
					tempTan[i][index1][k] = -ABC.y / ABC.x;
					tempTan[i][index2][k] = -ABC.y / ABC.x;
				}

				tempTan[i][index0] += glm::normalize(tempTan[i][index0]);
				tempTan[i][index1] += glm::normalize(tempTan[i][index1]);
				tempTan[i][index2] += glm::normalize(tempTan[i][index2]);
			}

			for (UINT j = 0; j < tempTan[i].size(); j++) {
				tempTan[i][j] = glm::normalize(tempTan[i][j]);
			}
		}
	}


	for (uint32_t i = 0; i < meshCount; i++) {
		uint32_t vCount = 0, iCount = 0, cnt = 0;

		vCount = (UINT)tempPos[i].size();
		iCount = (UINT)tempIndexArray[i].size();

		m_pos[materialMemory[i]].resize(m_vertexCount[materialMemory[i]] + vCount);
		m_nor[materialMemory[i]].resize(m_vertexCount[materialMemory[i]] + vCount);
		m_tex1[materialMemory[i]].resize(m_vertexCount[materialMemory[i]] + vCount);
		m_tex2[materialMemory[i]].resize(m_vertexCount[materialMemory[i]] + vCount);
		m_tan[materialMemory[i]].resize(m_vertexCount[materialMemory[i]] + vCount);
		m_weight[materialMemory[i]].resize(m_vertexCount[materialMemory[i]] + vCount);
		m_index[materialMemory[i]].resize(m_vertexCount[materialMemory[i]] + vCount);
		cnt = 0;
		for (uint32_t j = m_vertexCount[materialMemory[i]]; j < m_vertexCount[materialMemory[i]] + vCount; j++) {
			m_pos[materialMemory[i]][j] = tempPos[i][cnt];
			m_nor[materialMemory[i]][j] = tempNor[i][cnt];
			m_tex1[materialMemory[i]][j] = tempTex1[i][cnt];
			m_tex2[materialMemory[i]][j] = tempTex2[i][cnt];
			m_tan[materialMemory[i]][j] = tempTan[i][cnt];
			//m_weight[materialMemory[i]][j] = tempWeight[i][cnt];
			//m_index[materialMemory[i]][j] = tempIndex[i][cnt];
			cnt++;
		}

		m_indexArray[materialMemory[i]].resize(m_indexCount[materialMemory[i]] + iCount);

		cnt = 0;
		for (uint32_t j = m_indexCount[materialMemory[i]]; j < m_indexCount[materialMemory[i]] + iCount; j++) {
			m_indexArray[materialMemory[i]][j] = tempIndexArray[i][cnt] + m_vertexCount[materialMemory[i]];
			cnt++;
		}

		m_vertexCount[materialMemory[i]] += vCount;
		m_indexCount[materialMemory[i]] += iCount;


		m_allVertexCount += vCount;
		m_allIndexCount += iCount;
	}

	//uint32_t vCount = 0;
	//for (uint32_t i = 0; i < m_materialCount; i++) {
	//	for (auto& ite : m_indexArray[i]) {
	//		ite += vCount;
	//	}

	//	vCount += m_vertexCount[i];
	//}

	return true;
}

bool MeshLoader::loadMeshFromPmx(const char* filename) {
	return false;
}

void MeshLoader::destroy() {
	for (auto& ite : m_pos)
		vector<glm::vec3>().swap(ite);
	for (auto& ite : m_nor)
		vector<glm::vec3>().swap(ite);
	for (auto& ite : m_tex1)
		vector<glm::vec2>().swap(ite);
	for (auto& ite : m_tan)
		vector<glm::vec3>().swap(ite);
	for (auto& ite : m_tex2)
		vector<glm::vec2>().swap(ite);
	for (auto& ite : m_weight)
		vector<glm::vec4>().swap(ite);
	for (auto& ite : m_index)
		vector<glm::uvec4>().swap(ite);

	for (auto& ite : m_imageData)
		vector<unsigned char>().swap(ite);
}