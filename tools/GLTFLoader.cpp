#define TINYGLTF_IMPLEMENTATION

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <vector>
#include <set>
#include "../glm-master/glm/glm.hpp"
#include "../glm-master/glm/gtc/matrix_transform.hpp"
#include "../glm-master/glm/gtc/quaternion.hpp"

#include "GLTFLoader.h"

#include "../tools/tiny_gltf.h"


using namespace std;
using namespace tinygltf;

struct Vertex {
	vector<glm::vec3> pos;
	vector<glm::vec3> nor;
	vector<glm::vec2> tex;
	vector<glm::vec3> tan;
	vector<glm::ivec4> boneIndex;
	vector<glm::vec4> boneWeight;
	vector<UINT> index;
	uint32_t materialIndex;
};

struct MyMaterial {
	uint32_t albedoTexIndex;
	uint32_t normalTexIndex;
	uint32_t occrusionTexIndex;
	uint32_t pbrTexIndex;
};

struct MyImage {
	vector<unsigned char> rowdata;
	uint32_t componentCount;
	uint32_t width;
	uint32_t height;
	uint32_t bytePerPixel;
};

struct BoneInfo {
	int id = -1;
	int linearID = 0;
	vector<int> childs;
	int parentID = -1;
	glm::mat4 invBindMatrix = glm::mat4(1.0f);
};

struct Bone {
	int id = -1;
	Bone* firstChild = NULL;
	Bone* sibling = NULL;
};

struct BonePose {
	glm::vec3 trans;
	glm::quat quat;
	glm::vec3 scale;
};

void RecursiveNode(Node* node, Model& model, vector<int>* boneRelation, unordered_map<int, vector<int>>& bone) {
	for (size_t i = 0; i < node->children.size(); i++) {
		boneRelation->push_back(node->children[i]);
		RecursiveNode(&model.nodes[node->children[i]], model, &bone[node->children[i]], bone);
	}
}


bool GLTFLoader::LoadModel(const char* filename, bool skinned, bool binary) {
	Model model;
	TinyGLTF loader;
	string err;
	string warn;

	vector<Vertex> vertex;
	vector<MyMaterial> material;
	vector<MyImage> image;


	unordered_map<int, vector<int>> boneRelation;

	bool ret;

	if (!binary)
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	else
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);

	if (!ret)
		return false;

	uint32_t count = 0;
	uint32_t counter = 0;

	uint32_t allocSize = 0;

	for (auto ite : model.meshes) {
		count += ite.primitives.size();
	}

	vertex.resize(count);

	vector<glm::mat4> invBindMatrix;
	unordered_map<int, vector<BonePose>> animationMatrix;
	int keyFrame = 0;
	unordered_map<int, BoneInfo> boneInfo;
	unordered_map<int, Bone> boneTest;
	unordered_map<int, int> joints;
	unordered_map<int, BonePose> baseMatrix;
	glm::mat4 rootMatrix;

	if (model.skins.size() != 0) {
		for (uint32_t i = 0; i < model.skins.size(); i++) {
			const Skin& skin = model.skins[i];
			invBindMatrix.resize(skin.joints.size());
			const Accessor& accessor = model.accessors[skin.inverseBindMatrices];
			const BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const Buffer& buffer = model.buffers[bufferView.buffer];

			memcpy_s(invBindMatrix.data(), bufferView.byteLength, buffer.data.data(), bufferView.byteLength);

			for (size_t j = 0; j < skin.joints.size(); j++) {
				boneInfo[skin.joints[j]].invBindMatrix = invBindMatrix[j];
				boneInfo[skin.joints[j]].id = skin.joints[j];
				joints[skin.joints[j]] = (skin.joints[j]);
			}
		}

		//set<int> s(joints.begin(), joints.end());

		//joints = vector<int>(s.begin(), s.end());

		for (size_t i = 0; i < model.nodes.size(); i++) {
			boneInfo[i].linearID = (int)i;
			boneInfo[i].id = i;
		}

		RecursiveNode(&model.nodes[0], model, &boneRelation[0], boneRelation);
		for (auto ite = boneRelation.begin(); ite != boneRelation.end();) {
			if ((*ite).second.size() == 0) {
				ite = boneRelation.erase(ite);
				continue;
			}
			ite++;
		}
		for (auto& ite : boneInfo) {
			for (auto itr : boneRelation[ite.second.id]) {
				boneInfo[itr].parentID = ite.second.id;
			}
		}
		for (auto& ite : boneInfo) {
			ite.second.childs = boneRelation[ite.second.id];
		}

	}
	else {
		for (uint32_t i = 0; i < model.nodes.size(); i++) {

			boneInfo[i].invBindMatrix = glm::identity<glm::mat4>();
			boneInfo[i].id = i;
			joints[i] = i;
		}

		Node* parent = nullptr;
		uint32_t parentID = 0;
		for (auto& ite : model.nodes) {
			if (ite.name.find("Character1_Reference") != string::npos) {
				parent = &ite;
				break;
			}
			parentID++;
		}

		if (parent == nullptr)
			parentID = 0;

		RecursiveNode(&model.nodes[parentID], model, &boneRelation[parentID], boneRelation);
		for (auto ite = boneRelation.begin(); ite != boneRelation.end();) {
			if ((*ite).second.size() == 0) {
				ite = boneRelation.erase(ite);
				continue;
			}
			ite++;
		}
		for (auto& ite : boneInfo) {
			for (auto itr : boneRelation[ite.second.id]) {
				boneInfo[itr].parentID = ite.second.id;
			}
		}
		for (auto& ite : boneInfo) {
			ite.second.childs = boneRelation[ite.second.id];
		}
	}
	if (model.animations.size() != 0) {
		const Animation& animation = model.animations[0];

		unordered_map<int, vector<glm::vec3>> scale;
		unordered_map<int, vector<glm::quat>> rotate;
		unordered_map<int, vector<glm::vec3>> trans;
		for (auto ite : animation.channels) {
			const AnimationSampler& animationSampler = model.animations[0].samplers[ite.sampler];
			const Accessor& animationAccessor = model.accessors[animationSampler.output];
			const BufferView& bv = model.bufferViews[animationAccessor.bufferView];
			const Buffer& buf = model.buffers[bv.buffer];
			size_t offset = animationAccessor.byteOffset + bv.byteOffset;
			keyFrame = animationAccessor.count;
			if (ite.target_path.compare("rotation") == 0) {
				size_t stride = sizeof(glm::quat);
				glm::quat rot;
				for (uint32_t i = 0; i < animationAccessor.count; i++) {
					memcpy_s(&rot, stride, offset + buf.data.data() + i * stride, stride);
					rotate[ite.target_node].push_back(rot);
				}
			}
			else if (ite.target_path.compare("scale") == 0) {
				size_t stride = sizeof(glm::vec3);
				glm::vec3 scl;
				for (uint32_t i = 0; i < animationAccessor.count; i++) {
					memcpy_s(&scl, stride, offset + buf.data.data() + i * stride, stride);
					scale[ite.target_node].push_back(scl);
				}
			}
			else if (ite.target_path.compare("translation") == 0) {
				size_t stride = sizeof(glm::vec3);
				glm::vec3 trs;
				for (uint32_t i = 0; i < animationAccessor.count; i++) {
					memcpy_s(&trs, stride, offset + buf.data.data() + i * stride, stride);
					trans[ite.target_node].push_back(trs);
				}
			}
		}

		for (size_t i = 0; i < boneInfo.size(); i++) {
			animationMatrix[i].resize(keyFrame);
			const Node& node = model.nodes[i];
			for (int j = 0; j < keyFrame; j++) {
				if (trans.find(i) != trans.end()) {
					animationMatrix[i][j].trans = trans[i][j];
					animationMatrix[i][j].quat = rotate[i][j];
					animationMatrix[i][j].scale = scale[i][j];
				}
				else {
					animationMatrix[i][j].trans = glm::vec3(0.0f, 0.0f, 0.0f);
					animationMatrix[i][j].quat = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
					animationMatrix[i][j].scale = glm::vec3(1.0f, 1.0f, 1.0f);
				}
			}
		}
	}

	for (size_t i = 0; i < model.nodes.size(); i++) {
		const Node& node = model.nodes[i];
		glm::vec3 trs = glm::vec3(0.0f);
		glm::vec3 scl = glm::vec3(1.0f);
		glm::quat rot;
		rot.x = rot.z = rot.y = 0;
		rot.w = 1;
		if (node.translation.size() == 3) {
			trs.x = (float)node.translation[0];
			trs.y = (float)node.translation[1];
			trs.z = (float)node.translation[2];
		}
		if (node.scale.size() == 3) {
			scl.x = (float)node.scale[0];
			scl.y = (float)node.scale[1];
			scl.z = (float)node.scale[2];
		}
		if (node.rotation.size() == 4) {
			rot.x = (float)node.rotation[0];
			rot.y = (float)node.rotation[1];
			rot.z = (float)node.rotation[2];
			rot.w = (float)node.rotation[3];
		}
		if (node.matrix.size() == 16) {
			baseMatrix[i].trans = glm::vec3(0.0f, 0.0f, 0.0f);
			baseMatrix[i].quat = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
			baseMatrix[i].scale = glm::vec3(1.0f, 1.0f, 1.0f);
		}
		else {
			baseMatrix[i].trans = trs;
			baseMatrix[i].quat = rot;
			baseMatrix[i].scale = scl;
		}
	}
	if (model.nodes[0].matrix.size() == 16) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				rootMatrix[i][j] = (float)model.nodes[0].matrix[i * 4 + j];
			}
		}
	}
	else {
		rootMatrix = glm::translate(glm::identity<glm::mat4>(), baseMatrix[0].trans) *
			glm::mat4(baseMatrix[0].quat) *
			glm::scale(glm::identity<glm::mat4>(), baseMatrix[0].scale);
	}

	uint32_t boneMax = 0;

	m_meshCount = model.meshes.size();
	m_position.resize(m_meshCount);
	m_normal.resize(m_meshCount);
	m_uv.resize(m_meshCount);
	m_tangent.resize(m_meshCount);
	m_index.resize(m_meshCount);
	for (size_t i = 0; i < model.meshes.size(); i++) {
		const Mesh& mesh = model.meshes[i];
		for (size_t j = 0; j < mesh.primitives.size(); j++) {
			const Primitive& primitive = mesh.primitives[j];
			const Accessor& indexAccessor = model.accessors[primitive.indices];
			const BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
			const Buffer& index = model.buffers[indexView.buffer];

			vertex[counter].materialIndex = primitive.material;
			vertex[counter].materialIndex %= model.materials.size();

			for (auto& ite : primitive.attributes) {
				const Accessor& accessor = model.accessors[ite.second];
				const BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const Buffer& buffer = model.buffers[bufferView.buffer];

				size_t offset = (max)(accessor.byteOffset, bufferView.byteOffset);
				offset = accessor.byteOffset + bufferView.byteOffset;

				if (ite.first.compare("NORMAL") == 0) {
					glm::vec3 nor;
					for (size_t k = 0; k < accessor.count; k++) {
						memcpy_s(&nor, sizeof(glm::vec3), offset + buffer.data.data() + k * sizeof(glm::vec3), sizeof(glm::vec3));
						//						cout << "NOR:" << nor.x << "," << nor.y << "," << nor.z << endl;;
						vertex[counter].nor.push_back(nor);
					}
				}
				else if (ite.first.compare("POSITION") == 0) {
					glm::vec3 pos;
					for (size_t k = 0; k < accessor.count; k++) {
						memcpy_s(&pos, sizeof(glm::vec3), offset + buffer.data.data() + k * sizeof(glm::vec3), sizeof(glm::vec3));
						//						cout << "POS:" << pos.x << "," << pos.y << "," << pos.z << endl;;
						vertex[counter].pos.push_back(pos);
					}
				}
				else if (ite.first.compare("TEXCOORD_0") == 0) {
					glm::vec2 tex;
					for (size_t k = 0; k < accessor.count; k++) {
						memcpy_s(&tex, sizeof(glm::vec2), offset + buffer.data.data() + k * sizeof(glm::vec2), sizeof(glm::vec2));
						//						cout << "TEX:" << tex.x << "," << tex.y << endl;
						vertex[counter].tex.push_back(tex);
					}
				}
				else if (ite.first.compare("TANGENT") == 0) {
					glm::vec3 tan;
					for (size_t k = 0; k < accessor.count; k++) {
						memcpy_s(&tan, sizeof(glm::vec3), offset + buffer.data.data() + k * sizeof(glm::vec3), sizeof(glm::vec3));
						//						cout << "TAN:" << tan.x << "," << tan.y << "," << tan.z << endl;
						vertex[counter].tan.push_back(tan);
					}
				}
				else if (ite.first.compare("WEIGHTS_0") == 0) {
					glm::vec4 weight;
					for (size_t k = 0; k < accessor.count; k++) {
						memcpy_s(&weight, sizeof(glm::vec4), offset + buffer.data.data() + k * sizeof(glm::vec4), sizeof(glm::vec4));
						vertex[counter].boneWeight.push_back(weight);
					}
				}
				else if (ite.first.compare("JOINTS_0") == 0) {
					glm::ivec4 boneIndex;
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
						BYTE tmpIndex[4];
						for (size_t k = 0; k < accessor.count; k++) {
							memcpy_s(tmpIndex, sizeof(BYTE) * 4, offset + buffer.data.data() + k * sizeof(BYTE) * 4, sizeof(BYTE) * 4);
							boneIndex.x = (UINT)tmpIndex[0];
							boneIndex.y = (UINT)tmpIndex[1];
							boneIndex.z = (UINT)tmpIndex[2];
							boneIndex.w = (UINT)tmpIndex[3];
							vertex[counter].boneIndex.push_back(boneIndex);
							boneMax = (max)(boneMax, (uint32_t)boneIndex.x);
							boneMax = (max)(boneMax, (uint32_t)boneIndex.y);
							boneMax = (max)(boneMax, (uint32_t)boneIndex.z);
							boneMax = (max)(boneMax, (uint32_t)boneIndex.w);
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
							vertex[counter].boneIndex.push_back(boneIndex);
							boneMax = (max)(boneMax, (uint32_t)boneIndex.x);
							boneMax = (max)(boneMax, (uint32_t)boneIndex.y);
							boneMax = (max)(boneMax, (uint32_t)boneIndex.z);
							boneMax = (max)(boneMax, (uint32_t)boneIndex.w);
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
							vertex[counter].boneIndex.push_back(boneIndex);
						}
					}
				}
			}

			if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
				BYTE tmp;
				for (size_t k = 0; k < indexAccessor.count; k++) {
					memcpy_s(&tmp, sizeof(BYTE), index.data.data() + indexAccessor.byteOffset + k * sizeof(BYTE), sizeof(BYTE));
					//					cout << tmp << endl;
					vertex[counter].index.push_back((UINT)tmp);
				}
			}
			else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
				USHORT tmp;
				for (size_t k = 0; k < indexAccessor.count; k++) {
					memcpy_s(&tmp, sizeof(SHORT), index.data.data() + indexView.byteOffset + k * sizeof(SHORT), sizeof(SHORT));
					//					cout << tmp << endl;
					vertex[counter].index.push_back((UINT)tmp);
				}
			}
			else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
				UINT tmp;
				for (size_t k = 0; k < indexAccessor.count; k++) {
					memcpy_s(&tmp, sizeof(UINT), index.data.data() + indexView.byteOffset + k * sizeof(UINT), sizeof(UINT));
					//					cout << tmp << endl;
					vertex[counter].index.push_back(tmp);
				}
			}

			counter++;
		}
	}

	material.resize(model.materials.size());
	for (size_t i = 0; i < model.materials.size(); i++) {
		material[i].albedoTexIndex = model.materials[i].pbrMetallicRoughness.baseColorTexture.index;
		material[i].normalTexIndex = model.materials[i].normalTexture.index;
		material[i].occrusionTexIndex = model.materials[i].occlusionTexture.index;
		material[i].pbrTexIndex = model.materials[i].pbrMetallicRoughness.metallicRoughnessTexture.index;
	}

	image.resize(model.images.size());
	for (size_t i = 0; i < model.images.size(); i++) {
		image[i].bytePerPixel = model.images[i].bits / 8 * model.images[i].component;
		image[i].componentCount = model.images[i].component;
		image[i].width = model.images[i].width;
		image[i].height = model.images[i].height;
		image[i].rowdata = model.images[i].image;
	}

	for (auto& vertexIte : vertex) {
		UINT vertexCount = vertexIte.pos.size();
		vertexCount = max(vertexCount, vertexIte.nor.size());
		vertexCount = max(vertexCount, vertexIte.tex.size());
		vertexCount = max(vertexCount, vertexIte.tan.size());
		if (vertexIte.pos.size() < vertexCount) {
			UINT tmpSize = vertexIte.pos.size();
			vertexIte.pos.resize(vertexCount);
			for (UINT i = tmpSize; i < vertexCount; i++) {
				vertexIte.pos[i].x = vertexIte.pos[i].y = vertexIte.pos[i].z = 0.1f;
			}
		}
		if (vertexIte.nor.size() < vertexCount) {
			UINT tmpSize = vertexIte.nor.size();
			vertexIte.nor.resize(vertexCount);
			for (UINT i = tmpSize; i < vertexCount; i++) {
				vertexIte.nor[i].x = vertexIte.nor[i].y = vertexIte.nor[i].z = 0.1f;
			}
		}
		if (vertexIte.tex.size() < vertexCount) {
			UINT tmpSize = vertexIte.tex.size();
			vertexIte.tex.resize(vertexCount);
			for (UINT i = tmpSize; i < vertexCount; i++) {
				vertexIte.tex[i].x = vertexIte.tex[i].y = 1.0f;
			}
		}
		if (vertexIte.tan.size() < vertexCount) {
			UINT tmpSize = vertexIte.tan.size();
			vertexIte.tan.resize(vertexCount);
			for (UINT i = 0; i < vertexIte.tan.size(); i++) {
				vertexIte.tan[i] = glm::vec3(0.0f);
			}
			for (UINT i = 0; i < vertexIte.index.size(); i += 3) {
				UINT index0 = vertexIte.index[i + 0];
				UINT index1 = vertexIte.index[i + 1];
				UINT index2 = vertexIte.index[i + 2];
				if (index0 >= vertexIte.pos.size() || index1 >= vertexIte.pos.size() || index2 >= vertexIte.pos.size())
					continue;

				glm::vec3 cp0[3] = {
					glm::vec3(vertexIte.pos[index0].x, vertexIte.tex[index0].x, vertexIte.tex[index0].y),
					glm::vec3(vertexIte.pos[index0].y, vertexIte.tex[index0].x, vertexIte.tex[index0].y),
					glm::vec3(vertexIte.pos[index0].z, vertexIte.tex[index0].x, vertexIte.tex[index0].y),
				};
				glm::vec3 cp1[3] = {
					glm::vec3(vertexIte.pos[index1].x, vertexIte.tex[index1].x, vertexIte.tex[index1].y),
					glm::vec3(vertexIte.pos[index1].y, vertexIte.tex[index1].x, vertexIte.tex[index1].y),
					glm::vec3(vertexIte.pos[index1].z, vertexIte.tex[index1].x, vertexIte.tex[index1].y),
				};
				glm::vec3 cp2[3] = {
					glm::vec3(vertexIte.pos[index2].x, vertexIte.tex[index2].x, vertexIte.tex[index2].y),
					glm::vec3(vertexIte.pos[index2].y, vertexIte.tex[index2].x, vertexIte.tex[index2].y),
					glm::vec3(vertexIte.pos[index2].z, vertexIte.tex[index2].x, vertexIte.tex[index2].y),
				};

				for (int j = 0; j < 3; j++) {
					glm::vec3 v1 = cp1[j] - cp0[j];
					glm::vec3 v2 = cp2[j] - cp1[j];

					glm::vec3 ABC = glm::cross(v1, v2);

					vertexIte.tan[index0][j] = -ABC.y / ABC.x;
					vertexIte.tan[index1][j] = -ABC.y / ABC.x;
					vertexIte.tan[index2][j] = -ABC.y / ABC.x;
				}

				vertexIte.tan[index0] += glm::normalize(vertexIte.tan[index0]);
				vertexIte.tan[index1] += glm::normalize(vertexIte.tan[index1]);
				vertexIte.tan[index2] += glm::normalize(vertexIte.tan[index2]);
			}

			for (UINT i = 0; i < vertexIte.tan.size(); i += 3) {
				vertexIte.tan[i] = glm::normalize(vertexIte.tan[i]);
			}
		}
		if (vertexIte.boneWeight.size() < vertexCount) {
			UINT tmpSize = vertexIte.boneWeight.size();
			vertexIte.boneWeight.resize(vertexCount);
			for (UINT i = tmpSize; i < vertexCount; i++) {
				vertexIte.boneWeight[i].x = vertexIte.boneWeight[i].y = vertexIte.boneWeight[i].z = vertexIte.boneWeight[i].w = 0.0f;
				vertexIte.boneWeight[i].x = 1.0f;
			}
		}
		if (vertexIte.boneIndex.size() < vertexCount) {
			UINT tmpSize = vertexIte.boneIndex.size();
			vertexIte.boneIndex.resize(vertexCount);
			for (UINT i = tmpSize; i < vertexCount; i++) {
				vertexIte.boneIndex[i].x = vertexIte.boneIndex[i].y = vertexIte.boneIndex[i].z = vertexIte.boneIndex[i].w = 0;
			}
		}
	}

	vector<glm::vec4>().swap(m_position);
	vector<glm::vec4>().swap(m_normal);
	vector<glm::vec4>().swap(m_uv);
	for (uint32_t i = 0; i < m_meshCount; i++) {
		for (uint32_t j = 0; j < vertex[i].index.size(); j++) {
			m_position.push_back(glm::vec4(vertex[i].pos[vertex[i].index[j]], 0));
			m_normal.push_back(glm::vec4(vertex[i].nor[vertex[i].index[j]], 0));
			m_uv.push_back(glm::vec4(vertex[i].tex[vertex[i].index[j]], 1, 1));
		}
	}

	uint32_t dataSize = 0;

	uint32_t meshCount = vertex.size();
	dataSize += sizeof(uint32_t);


	uint32_t materialCount = material.size();
	dataSize += sizeof(uint32_t);

	for (auto ite : vertex) {
		dataSize += sizeof(uint32_t);
	}

	for (auto ite : vertex) {
		uint32_t vertexCount = ite.pos.size();
		uint32_t indexCount = ite.index.size();
		dataSize += sizeof(uint32_t);
		for (uint32_t i = 0; i < vertexCount; i++) {
			dataSize += sizeof(glm::vec3);
			dataSize += sizeof(glm::vec3);
			dataSize += sizeof(glm::vec2);
			dataSize += sizeof(glm::vec3);
			if (skinned) {
				dataSize += sizeof(glm::vec4);
				dataSize += sizeof(glm::vec4);
			}
		}
		dataSize += sizeof(uint32_t);
		dataSize += sizeof(uint32_t) * indexCount;
	}

	for (auto ite : material) {
		dataSize += sizeof(uint32_t);
		dataSize += sizeof(uint32_t);
		dataSize += sizeof(uint32_t);
		dataSize += sizeof(uint32_t);
	}

	uint32_t imageCount = image.size();
	dataSize += sizeof(uint32_t);
	for (auto ite : image) {
		dataSize += sizeof(uint32_t);
		dataSize += sizeof(uint32_t);
		dataSize += sizeof(uint32_t);
		dataSize += ite.width * ite.height * ite.bytePerPixel;
	}

	if (skinned) {
		uint32_t boneCount = boneInfo.size();
		uint32_t jointsCount = joints.size();
		dataSize += sizeof(uint32_t);
		for (uint32_t i = 0; i < joints.size(); i++) {
			dataSize += sizeof(uint32_t);
		}
		dataSize += sizeof(uint32_t);
		dataSize += sizeof(uint32_t);
		for (uint32_t i = 0; i < boneCount; i++) {
			dataSize += sizeof(uint32_t);
			dataSize += sizeof(uint32_t);
			dataSize += sizeof(uint32_t);
			size_t childNum = boneInfo[i].childs.size();
			dataSize += sizeof(uint32_t);
			for (size_t j = 0; j < childNum; j++)
				dataSize += sizeof(uint32_t);
			dataSize += sizeof(glm::mat4);
			dataSize += sizeof(glm::vec3);
			dataSize += sizeof(glm::quat);
			dataSize += sizeof(glm::vec3);
		}

		dataSize += sizeof(glm::mat4);

		uint32_t animCount = (uint32_t)keyFrame;
		dataSize += sizeof(uint32_t);
		for (uint32_t i = 0; i < boneCount; i++) {
			for (uint32_t j = 0; j < animCount; j++) {
				dataSize += sizeof(glm::vec3);
				dataSize += sizeof(glm::quat);
				dataSize += sizeof(glm::vec3);
			}
		}
	}

	m_data.resize(dataSize);
	uint32_t offset = 0;

	memcpy_s(m_data.data() + offset, sizeof(uint32_t), &meshCount, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy_s(m_data.data() + offset, sizeof(uint32_t), &materialCount, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	for (auto ite : vertex) {
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &ite.materialIndex, sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	for (auto ite : vertex) {
		uint32_t vertexCount = ite.pos.size();
		uint32_t indexCount = ite.index.size();
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &vertexCount, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		for (uint32_t i = 0; i < vertexCount; i++) {
			memcpy_s(m_data.data() + offset, sizeof(glm::vec3), &ite.pos[i], sizeof(glm::vec3));
			offset += sizeof(glm::vec3);
			memcpy_s(m_data.data() + offset, sizeof(glm::vec3), &ite.nor[i], sizeof(glm::vec3));
			offset += sizeof(glm::vec3);
			memcpy_s(m_data.data() + offset, sizeof(glm::vec2), &ite.tex[i], sizeof(glm::vec2));
			offset += sizeof(glm::vec2);
			memcpy_s(m_data.data() + offset, sizeof(glm::vec3), &ite.tan[i], sizeof(glm::vec3));
			offset += sizeof(glm::vec3);
			if (skinned) {
				memcpy_s(m_data.data() + offset, sizeof(glm::vec4), &ite.boneWeight[i], sizeof(glm::vec4));
				offset += sizeof(glm::vec4);
				memcpy_s(m_data.data() + offset, sizeof(glm::vec4), &ite.boneIndex[i], sizeof(glm::vec4));
				offset += sizeof(glm::vec4);
			}
		}
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &indexCount, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy_s(m_data.data() + offset, sizeof(uint32_t) * indexCount, ite.index.data(), sizeof(uint32_t) * indexCount);
		offset += sizeof(uint32_t) * indexCount;
	}

	for (auto ite : material) {
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &ite.albedoTexIndex, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &ite.normalTexIndex, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &ite.occrusionTexIndex, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &ite.pbrTexIndex, sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	memcpy_s(m_data.data() + offset, sizeof(uint32_t), &imageCount, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	for (auto ite : image) {
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &ite.width, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &ite.height, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &ite.bytePerPixel, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy_s(m_data.data() + offset, ite.width * ite.height * ite.bytePerPixel, ite.rowdata.data(), ite.width * ite.height * ite.bytePerPixel);
		offset += ite.width * ite.height * ite.bytePerPixel;
	}

	if (skinned) {
		uint32_t boneCount = boneInfo.size();
		uint32_t jointsCount = joints.size();
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &jointsCount, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		for (auto ite : joints) {
			memcpy_s(m_data.data() + offset, sizeof(uint32_t), &ite, sizeof(uint32_t));
			offset += sizeof(uint32_t);
		}
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &boneCount, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &jointsCount, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		for (uint32_t i = 0; i < boneCount; i++) {
			memcpy_s(m_data.data() + offset, sizeof(uint32_t), &boneInfo[i].id, sizeof(uint32_t));
			offset += sizeof(uint32_t);
			memcpy_s(m_data.data() + offset, sizeof(uint32_t), &boneInfo[i].linearID, sizeof(uint32_t));
			offset += sizeof(uint32_t);
			memcpy_s(m_data.data() + offset, sizeof(uint32_t), &boneInfo[i].parentID, sizeof(uint32_t));
			offset += sizeof(uint32_t);
			size_t childNum = boneInfo[i].childs.size();
			memcpy_s(m_data.data() + offset, sizeof(uint32_t), &childNum, sizeof(uint32_t));
			offset += sizeof(uint32_t);
			for (size_t j = 0; j < childNum; j++) {
				memcpy_s(m_data.data() + offset, sizeof(uint32_t), &boneInfo[i].childs[j], sizeof(uint32_t));
				offset += sizeof(uint32_t);
			}
			memcpy_s(m_data.data() + offset, sizeof(glm::mat4), &boneInfo[i].invBindMatrix, sizeof(glm::mat4));
			offset += sizeof(glm::mat4);
			memcpy_s(m_data.data() + offset, sizeof(glm::vec3), &baseMatrix[i].trans, sizeof(glm::vec3));
			offset += sizeof(glm::vec3);
			memcpy_s(m_data.data() + offset, sizeof(glm::quat), &baseMatrix[i].quat, sizeof(glm::quat));
			offset += sizeof(glm::quat);
			memcpy_s(m_data.data() + offset, sizeof(glm::vec3), &baseMatrix[i].scale, sizeof(glm::vec3));
			offset += sizeof(glm::vec3);
		}

		memcpy_s(m_data.data() + offset, sizeof(glm::mat4), &rootMatrix, sizeof(glm::mat4));
		offset += sizeof(glm::mat4);

		uint32_t animCount = (uint32_t)keyFrame;
		memcpy_s(m_data.data() + offset, sizeof(uint32_t), &animCount, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		for (uint32_t i = 0; i < boneCount; i++) {
			for (uint32_t j = 0; j < animCount; j++) {
				memcpy_s(m_data.data() + offset, sizeof(glm::vec3), &animationMatrix[i][j].trans, sizeof(glm::vec3));
				offset += sizeof(glm::vec3);
				memcpy_s(m_data.data() + offset, sizeof(glm::quat), &animationMatrix[i][j].quat, sizeof(glm::quat));
				offset += sizeof(glm::quat);
				memcpy_s(m_data.data() + offset, sizeof(glm::vec3), &animationMatrix[i][j].scale, sizeof(glm::vec3));
				offset += sizeof(glm::vec3);
			}
		}
	}


	return true;
}