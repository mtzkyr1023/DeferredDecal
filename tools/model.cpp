#include "model.h"
#include "mesh_loader.h"
#include "xatlas.h"
#include "../render_pass/render_pass.h"
#include "GLTFLoader.h"

#include <fstream>

using namespace std;

Model::Model() {

}

Model::~Model() {

}

bool Model::createFromGltf(ID3D12Device* dev, ID3D12CommandQueue* queue, uint32_t bufferCount,
	const char* filename, bool binary) {
	MeshLoader loader;

	m_filename = filename;
	m_filename.insert(7, "cache/");
	m_filename += ".cache";
	
	{
		BYTE temp[4 * 4 * 4] = {
			255, 255, 255, 255,
			255, 255, 255, 255,
			255, 255, 255, 255,
			255, 255, 255, 255
		};

		m_defaultImage = ResourceManager::Instance().createTexture(dev, queue, 1, 4, 4, 4, temp, false);
	}

	if (loadModelCache(dev, queue, bufferCount))
		return true;
	
	if (!loader.loadMeshFromGltf(filename, binary))
		return false;

	m_min = glm::vec3(FLT_MAX);
	m_max = glm::vec3(-FLT_MAX);

	vector<uint32_t> index;
	vector<glm::vec3> posArray;

	m_positionArray.reserve((loader.getAllVertexCount()));
	m_normalArray.reserve((loader.getAllVertexCount()));
	m_tangentArray.reserve(loader.getAllVertexCount());
	m_uvArray.reserve(loader.getAllVertexCount());

	m_materialCount = loader.getMaterialCount();


	m_albedoImageIndex.resize(m_materialCount);
	m_normalImageIndex.resize(m_materialCount);
	m_roughMetalImageIndex.resize(m_materialCount);

	m_vertexArray.reserve(loader.getAllVertexCount());

	m_indexArray.reserve(loader.getAllIndexCount());

	std::vector<uint32_t> indexOffsetArray;

	int indexBufferOffset = 0;
	for (uint32_t i = 0; i < loader.getMaterialCount(); i++) {
		for (uint32_t j = 0; j < loader.getVertexCount()[i]; j++) {
			glm::vec4 pos = glm::vec4(loader.getPos(i)[j], 1.0f);
			glm::vec4 nor = glm::vec4(loader.getNor(i)[j], 0.0f);
			glm::vec4 tan = glm::vec4(loader.getTan(i)[j], 0.0f);
			glm::vec4 tex = glm::vec4(loader.getTex1(i)[j], 0.0f, 0.0f);
			m_positionArray.push_back(pos);
			m_normalArray.push_back(nor);
			m_tangentArray.push_back(tan);
			m_uvArray.push_back(tex);

			m_vertexArray.emplace_back(
				loader.getPos(i)[j],
				loader.getNor(i)[j],
				loader.getTan(i)[j],
				loader.getTex1(i)[j]);
			
			m_min = glm::min(m_min, glm::vec3(pos.x, pos.y, pos.z));
			m_max = glm::max(m_max, glm::vec3(pos.x, pos.y, pos.z));
		}

		indexBufferOffset += i != 0 ? m_vertexCount[i - 1] : 0;
		for (uint32_t j = 0; j < loader.getIndexCount()[i]; j++) {
			m_indexArray.push_back(loader.getIndexArray(i)[j] + indexBufferOffset);
		}

		m_indexCount.push_back(loader.getIndexCount()[i]);
		m_vertexCount.push_back(loader.getVertexCount()[i]);

		m_albedoImageIndex[i] = loader.getAlbedoImageIndex(i);
		m_normalImageIndex[i] = loader.getNormalImageIndex(i);
		m_roughMetalImageIndex[i] = loader.getPbrImageIndex(i);

		if (m_indexArray.size() == 0) {
			continue;
		}
		if (i != 0) {
			indexOffsetArray.push_back(m_indexArray.back());
		}
		else {
			indexOffsetArray.push_back(0);
		}
	}

	uint32_t vertexOffset = 0;
	uint32_t indexOffset = 0;


	uint32_t instanceIndexOffset = 0;

	std::vector<uint32_t> offsetBuffer;

	for (int i = 0; i < m_materialCount; i++) {
		glm::vec3 aabbmin(FLT_MAX, FLT_MAX, FLT_MAX);
		glm::vec3 aabbmax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		instanceIndexOffset = 0;
		for (int j = 0; j < m_indexCount[i]; j++) {
			aabbmin = glm::min(aabbmin, glm::vec3(m_positionArray[m_indexArray[i]]));
			aabbmax = glm::max(aabbmax, glm::vec3(m_positionArray[m_indexArray[i]]));
			if (j != 0 && ((j / 3) % 256 == 0 || j == m_indexCount[i] - 1)) {

				Instance instance{};
				instance.aabbmax = aabbmax;
				instance.aabbmin = aabbmin;
				instance.indexOffset = instanceIndexOffset;
				instance.indexCount = j - instanceIndexOffset;
				m_instanceArray.push_back(instance);

				instanceIndexOffset = j;

				aabbmin = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
				aabbmax = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			}
		}

		offsetBuffer.push_back(indexOffset);

		vertexOffset += m_vertexCount[i];
		indexOffset += m_indexCount[i];
	}

	m_vertexBuffer = ResourceManager::Instance().createVertexBuffer(dev, queue, 1, sizeof(Vertex), sizeof(Vertex) * m_vertexArray.size(), m_vertexArray.data());
	m_indexBuffer = ResourceManager::Instance().createIndexBuffer(dev, queue, 1, (UINT)sizeof(uint32_t), (UINT)m_indexArray.size() * sizeof(int), m_indexArray.data());

	uint32_t currentIndexCount = 0;

	for (int i = 0; i < m_materialCount; i++) {
		//glm::vec3 aabbmin(FLT_MAX, FLT_MAX, FLT_MAX);
		//glm::vec3 aabbmax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		//instanceIndexOffset++;
		//for (int j = 0; j < m_indexCount[i]; j++,currentIndexCount++) {
		//	aabbmin = glm::min(aabbmin, glm::vec3(m_positionArray[m_indexArray[currentIndexCount]]));
		//	aabbmax = glm::max(aabbmax, glm::vec3(m_positionArray[m_indexArray[currentIndexCount]]));
		//	if (j != 0 && ((j / 3) % 256 == 0 || j == m_indexCount[i] - 1)) {


		//		m_instanceArray.emplace_back(aabbmin, aabbmax, instanceIndexOffset, j % 256, i);


		//		aabbmin = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		//		aabbmax = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		//		instanceIndexOffset = j;
		//	}
		//}

		vertexOffset += m_vertexCount[i];
		indexOffset += m_indexCount[i];
	}

	m_instanceBuffer = ResourceManager::Instance().createStructuredBuffer(dev, queue, 1, sizeof(Instance), m_instanceArray.size(), m_instanceArray.data());

	m_images.resize(loader.getImageCount());

	m_imageCount = loader.getImageCount();
	m_imageDatas.resize(loader.getImageCount());
	m_images.resize(loader.getImageCount());
	m_imageWidths.resize(loader.getImageCount());
	m_imageHeights.resize(loader.getImageCount());
	for (uint32_t i = 0; i < loader.getImageCount(); i++) {
		m_images[i] = ResourceManager::Instance().createTexture(dev, queue, 1, loader.getImageWidth(i), loader.getImageHeight(i), 4, loader.getImageData(i).data(), false);
		m_imageWidths[i] = loader.getImageWidth(i);
		m_imageHeights[i] = loader.getImageHeight(i);
		m_imageDatas[i] = loader.getImageData(i);
	}

	m_allIndexCount = (UINT)m_indexArray.size();
	m_allVertexCount = (UINT)m_positionArray.size();

	//if (!calculateUV(dev, queue))
	//	return false;

	saveModelCache();


	return true;
}

bool Model::loadModelCache(ID3D12Device* device, ID3D12CommandQueue* queue, uint32_t bufferCount) {
	ifstream ifs;

	ifs.open(m_filename.c_str(), ios_base::binary);
	if (ifs.fail())
		return false;

	ifs.read((char*)&m_materialCount, sizeof(uint32_t));
	ifs.read((char*)&m_allVertexCount, sizeof(uint32_t));
	ifs.read((char*)&m_allIndexCount, sizeof(uint32_t));
	m_positionArray.resize(m_allVertexCount);
	m_normalArray.resize(m_allVertexCount);
	m_tangentArray.resize(m_allVertexCount);
	m_uvArray.resize(m_allVertexCount);
	m_indexArray.resize(m_allIndexCount);
	m_vertexArray.resize(m_allVertexCount);
	ifs.read((char*)m_positionArray.data(), sizeof(glm::vec4) * m_allVertexCount);
	ifs.read((char*)m_normalArray.data(), sizeof(glm::vec4) * m_allVertexCount);
	ifs.read((char*)m_tangentArray.data(), sizeof(glm::vec4) * m_allVertexCount);
	ifs.read((char*)m_uvArray.data(), sizeof(glm::vec4) * m_allVertexCount);
	ifs.read((char*)m_indexArray.data(), sizeof(uint32_t) * m_allIndexCount);


	for (int i = 0; i < m_allVertexCount; i++)
	{
		m_vertexArray[i].pos = m_positionArray[i];
		m_vertexArray[i].nor = m_normalArray[i];
		m_vertexArray[i].tan = m_tangentArray[i];
		m_vertexArray[i].tex = m_uvArray[i];
	}

	m_vertexCount.resize(m_materialCount);
	m_indexCount.resize(m_materialCount);
	m_albedoImageIndex.resize(m_materialCount);
	m_normalImageIndex.resize(m_materialCount);
	m_roughMetalImageIndex.resize(m_materialCount);
	vector<glm::uvec4> instanceToIndexMap;
	for (uint32_t i = 0; i < m_materialCount; i++) {
		ifs.read((char*)&m_vertexCount[i], sizeof(uint32_t));
		ifs.read((char*)&m_indexCount[i], sizeof(uint32_t));
		ifs.read((char*)&m_albedoImageIndex[i], sizeof(uint32_t));
		ifs.read((char*)&m_normalImageIndex[i], sizeof(uint32_t));
		ifs.read((char*)&m_roughMetalImageIndex[i], sizeof(uint32_t));
	}

	uint32_t vertexOffset = 0;
	uint32_t indexOffset = 0;


	const uint32_t divIndexCount = 128 * 3;

	uint32_t instanceIndexOffset = 0;

	std::vector<uint32_t> offsetBuffer;

	{
		int counter = 0;
		auto ite = m_indexCount.begin();
		int materialIndex = 0;
		int offset = 0;
		glm::vec3 aabbmin(FLT_MAX, FLT_MAX, FLT_MAX);
		glm::vec3 aabbmax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		for (int j = 0; j < m_allIndexCount; j += 3) {
			aabbmin = glm::min(aabbmin, glm::vec3(m_positionArray[m_indexArray[j]]));
			aabbmax = glm::max(aabbmax, glm::vec3(m_positionArray[m_indexArray[j]]));
			if (j != 0 && ((j) % divIndexCount == 0)) {

				Instance instance{};
				instance.aabbmax = aabbmax;
				instance.aabbmin = aabbmin;
				instance.indexOffset = instanceIndexOffset;
				instance.indexCount = j - instanceIndexOffset;

				int materialIdBuffer = ResourceManager::Instance().createConstantBuffer(device, sizeof(glm::mat4) * 4, 1);
				ResourceManager::Instance().getResourceAsCB(materialIdBuffer)->updateBuffer(0, sizeof(UINT), &counter);
				if (j >= (*ite) + offset) {
					materialIndex++;
					offset += (*ite);
					ite++;
				}
				instanceToIndexMap.push_back(glm::uvec4(materialIndex));
				counter++;

				instance.materialId = ResourceManager::Instance().getResource(materialIdBuffer)->getResource(0)->GetGPUVirtualAddress();
				m_materialIdBuffer.push_back(materialIdBuffer);

				m_instanceArray.push_back(instance);

				instanceIndexOffset = j;

				aabbmin = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
				aabbmax = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			}
			else if (j == m_allIndexCount - 1) {
				Instance instance{};
				instance.aabbmax = aabbmax;
				instance.aabbmin = aabbmin;
				instance.indexOffset = instanceIndexOffset;
				instance.indexCount = j - instanceIndexOffset;

				materialIndex++;
				instanceToIndexMap.push_back(glm::uvec4(materialIndex));

				int materialIdBuffer = ResourceManager::Instance().createConstantBuffer(device, sizeof(glm::mat4) * 4, 1);
				ResourceManager::Instance().getResourceAsCB(materialIdBuffer)->updateBuffer(0, sizeof(UINT), &counter);
				counter++;

				instance.materialId = ResourceManager::Instance().getResource(materialIdBuffer)->getResource(0)->GetGPUVirtualAddress();
				m_materialIdBuffer.push_back(materialIdBuffer);

				m_instanceArray.push_back(instance);

				instanceIndexOffset = j;

				aabbmin = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
				aabbmax = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			}
		}
	}

	m_instanceToIndexMap = ResourceManager::Instance().createStructuredBuffer(device, queue, 1, sizeof(glm::uvec4), instanceToIndexMap.size(), instanceToIndexMap.data());

	m_instanceBuffer = ResourceManager::Instance().createStructuredBuffer(device, queue, 1, sizeof(Instance), m_instanceArray.size(), m_instanceArray.data());

	m_vertexBuffer = ResourceManager::Instance().createVertexBuffer(device, queue, 1, sizeof(Vertex), sizeof(Vertex) * m_allVertexCount, m_vertexArray.data());
	m_indexBuffer = ResourceManager::Instance().createIndexBuffer(device, queue, 1, (UINT)sizeof(uint32_t), (UINT)m_indexArray.size() * sizeof(int), m_indexArray.data());

	ifs.read((char*)&m_imageCount, sizeof(uint32_t));
	m_imageDatas.resize(m_imageCount);
	m_imageWidths.resize(m_imageCount);
	m_imageHeights.resize(m_imageCount);
	m_images.resize(m_imageCount);
	for (uint32_t i = 0; i < m_imageCount; i++) {
		ifs.read((char*)&m_imageWidths[i], sizeof(uint32_t));
		ifs.read((char*)&m_imageHeights[i], sizeof(uint32_t));
		m_imageDatas[i].resize(m_imageWidths[i] * m_imageHeights[i] * 4);
		ifs.read((char*)m_imageDatas[i].data(), m_imageWidths[i] * m_imageHeights[i] * 4);
 		m_images[i] = ResourceManager::Instance().createTexture(device, queue, 1, m_imageWidths[i], m_imageHeights[i], 4, m_imageDatas[i].data(), false);

		vector<unsigned char>().swap(m_imageDatas[i]);
	}

	m_texcoord.resize(m_allIndexCount);
	ifs.read((char*)m_texcoord.data(), sizeof(glm::vec2) * m_texcoord.size());

	ifs.close();



	return true;
}

void Model::saveModelCache() {
	ofstream ofs;
	ofstream txt;
	string textFilename = m_filename + ".txt";

	ofs.open(m_filename.c_str(), ios_base::binary);
	if (ofs.fail())
		return;
	txt.open(textFilename.c_str());
	ofs.write((const char*)&m_materialCount, sizeof(uint32_t));
	ofs.write((const char*)&m_allVertexCount, sizeof(uint32_t));
	ofs.write((const char*)&m_allIndexCount, sizeof(uint32_t));
	ofs.write((const char*)m_positionArray.data(), sizeof(glm::vec4) * m_allVertexCount);
	ofs.write((const char*)m_normalArray.data(), sizeof(glm::vec4) * m_allVertexCount);
	ofs.write((const char*)m_tangentArray.data(), sizeof(glm::vec4) * m_allVertexCount);
	ofs.write((const char*)m_uvArray.data(), sizeof(glm::vec4) * m_allVertexCount);
	ofs.write((const char*)m_indexArray.data(), sizeof(uint32_t) * m_allIndexCount);
	txt << "MaterialCount:" << m_materialCount << endl;
	txt << "AllVertexCount:" << m_allVertexCount << endl;
	txt << "AllIndexCount:" << m_allIndexCount << endl;
	for (uint32_t i = 0; i < m_materialCount; i++) {
		ofs.write((const char*)&m_vertexCount[i], sizeof(uint32_t));
		ofs.write((const char*)&m_indexCount[i], sizeof(uint32_t));
		ofs.write((const char*)&m_albedoImageIndex[i], sizeof(uint32_t));
		ofs.write((const char*)&m_normalImageIndex[i], sizeof(uint32_t));
		ofs.write((const char*)&m_roughMetalImageIndex[i], sizeof(uint32_t));
	}

	ofs.write((const char*)&m_imageCount, sizeof(uint32_t));
	txt << "ImageCount:" << m_imageCount << endl;
	for (uint32_t i = 0; i < m_imageCount; i++) {
		ofs.write((const char*)&m_imageWidths[i], sizeof(uint32_t));
		ofs.write((const char*)&m_imageHeights[i], sizeof(uint32_t));
		ofs.write((const char*)m_imageDatas[i].data(), m_imageDatas[i].size());
		txt << "image[" << i << "]:" << endl;
		txt << "	width:" << m_imageWidths[i] << endl;
		txt << "	height:" << m_imageHeights[i] << endl;
	}

	ofs.write((const char*)m_texcoord.data(), sizeof(glm::vec2) * m_texcoord.size());

	ofs.close();
	txt.close();
}


bool Model::calculateUV(ID3D12Device* dev, ID3D12CommandQueue* queue) {
	xatlas::MeshDecl decl{};
	decl.indexCount = m_indexArray.size();
	decl.indexData = m_indexArray.data();
	decl.indexFormat = xatlas::IndexFormat::UInt32;
	decl.vertexCount = m_allVertexCount;
	decl.vertexPositionData = m_positionArray.data();
	decl.vertexPositionStride = sizeof(glm::vec4);
	decl.vertexNormalData = m_normalArray.data();
	decl.vertexNormalStride = sizeof(glm::vec4);
	decl.vertexUvStride = sizeof(glm::vec2);
	decl.vertexUvData = m_uvArray.data();
	xatlas::Atlas* atlas = xatlas::Create();
	xatlas::AddMesh(atlas, decl, 0);
	xatlas::ChartOptions chartOpt{};
	chartOpt.fixWinding = false;
	chartOpt.useInputMeshUvs = true;
	xatlas::PackOptions packOpt{};
	packOpt.bilinear = true;
	packOpt.bruteForce = false;
	packOpt.resolution = 0;
	packOpt.texelsPerUnit = 0.0f;
	packOpt.padding = 0;
	xatlas::Generate(atlas);

	xatlas::Mesh& mesh = atlas->meshes[0];
	m_texcoord.resize(mesh.indexCount);
	for (uint32_t i = 0; i < m_allIndexCount; i += 3) {
		m_texcoord[i + 0].x = mesh.vertexArray[mesh.indexArray[i + 0]].uv[0];
		m_texcoord[i + 1].x = mesh.vertexArray[mesh.indexArray[i + 1]].uv[0];
		m_texcoord[i + 2].x = mesh.vertexArray[mesh.indexArray[i + 2]].uv[0];
		m_texcoord[i + 0].y = mesh.vertexArray[mesh.indexArray[i + 0]].uv[1];
		m_texcoord[i + 1].y = mesh.vertexArray[mesh.indexArray[i + 1]].uv[1];
		m_texcoord[i + 2].y = mesh.vertexArray[mesh.indexArray[i + 2]].uv[1];
	}

	xatlas::Destroy(atlas);

	vector<glm::vec4> tex;
	for (uint32_t i = 0; i < m_texcoord.size(); i++) {
		tex.push_back(glm::vec4(m_texcoord[i], 0.0f, 0.0f));
	}

	return true;
}


void Model::createBundle(ID3D12Device* dev, ID3D12RootSignature* rootSignature) {

}


bool MeshManager::createMeshFromGltf(ID3D12Device* dev, ID3D12CommandQueue* queue,
	uint32_t bufferCount, const char* filename, bool binary) {
	if (m_meshArray.find(filename) == m_meshArray.end()) {
		m_meshArray[filename] = std::make_unique<Model>();
	}
	if (!m_meshArray[filename]->createFromGltf(dev, queue, bufferCount, filename, binary))
		return false;

	return true;
}

void MeshManager::releaseMesh(const char* filename) {
	m_meshArray.erase(filename);
}

void MeshManager::allRelease() {
	m_meshArray.clear();
}