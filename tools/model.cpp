#include "model.h"
#include "mesh_loader.h"

#include <fstream>

using namespace std;

Mesh::Mesh() {

}

Mesh::~Mesh() {

}

bool Mesh::createFromGltf(ID3D12Device* dev, ID3D12CommandQueue* queue, uint32_t bufferCount,
	const char* filename, bool binary) {
	MeshLoader loader;

	m_filename = filename;
	m_filename.insert(7, "cache/");
	m_filename += ".cache";

	if (loadModelCache(dev, queue, bufferCount))
		return true;
	
	if (!loader.loadMeshFromGltf(filename, binary))
		return false;

	m_min = glm::vec3(FLT_MAX);
	m_max = glm::vec3(-FLT_MAX);

	vector<uint32_t> index;

	m_vertexArray.reserve((loader.getAllVertexCount()));

	m_materialCount = loader.getMaterialCount();


	m_albedoImageIndex.resize(m_materialCount);
	m_normalImageIndex.resize(m_materialCount);
	m_roughMetalImageIndex.resize(m_materialCount);

	for (uint32_t i = 0; i < loader.getMaterialCount(); i++) {
		for (uint32_t j = 0; j < loader.getVertexCount()[i]; j++) {
			Vertex tmp;
			tmp.pos = loader.getPos(i)[j];
			tmp.nor = loader.getNor(i)[j];
			tmp.tex = loader.getTex1(i)[j];
			tmp.tan = loader.getTan(i)[j];
			m_vertexArray.push_back(tmp);
			
			m_min = glm::min(m_min, tmp.pos);
			m_max = glm::max(m_max, tmp.pos);
		}
		index.insert(index.end(), loader.getIndexArray(i).begin(), loader.getIndexArray(i).end());

		m_indexCount.push_back(loader.getIndexCount()[i]);
		m_vertexCount.push_back(loader.getVertexCount()[i]);

		m_albedoImageIndex[i] = loader.getAlbedoImageIndex(i);
		m_normalImageIndex[i] = loader.getNormalImageIndex(i);
		m_roughMetalImageIndex[i] = loader.getPbrImageIndex(i);
	}

	for (uint32_t i = 0; i < m_vertexArray.size(); i++) {
		m_posArray.push_back(m_vertexArray[i].pos);
	}
	for (uint32_t i = 0; i < index.size(); i++) {
		m_indexArray.push_back(index[i]);
	}

	m_vertexBuffer.create(dev, queue, bufferCount, (UINT)sizeof(Vertex), (UINT)(sizeof(Vertex) * (UINT)m_vertexArray.size()), m_vertexArray.data());
	m_indexBuffer.create(dev, queue, bufferCount, (UINT)(sizeof(uint32_t) * index.size()), index.data());

	m_images.resize(loader.getImageCount());

	m_imageCount = loader.getImageCount();
	m_imageDatas.resize(loader.getImageCount());
	m_images.resize(loader.getImageCount());
	m_imageWidths.resize(loader.getImageCount());
	m_imageHeights.resize(loader.getImageCount());
	for (uint32_t i = 0; i < loader.getImageCount(); i++) {
		if (!m_images[i].createResource(dev, queue, bufferCount,
			loader.getImageWidth(i), loader.getImageHeight(i), 4, loader.getImageData(i).data(), true))
			return false;
		m_imageWidths[i] = loader.getImageWidth(i);
		m_imageHeights[i] = loader.getImageHeight(i);
		m_imageDatas[i] = loader.getImageData(i);
	}

	m_allIndexCount = (UINT)index.size();
	m_allVertexCount = (UINT)m_vertexArray.size();

	saveModelCache();

	return true;
}

bool Mesh::loadModelCache(ID3D12Device* device, ID3D12CommandQueue* queue, uint32_t bufferCount) {
	ifstream ifs;

	ifs.open(m_filename.c_str(), ios_base::binary);
	if (ifs.fail())
		return false;

	ifs.read((char*)&m_materialCount, sizeof(uint32_t));
	ifs.read((char*)&m_allVertexCount, sizeof(uint32_t));
	ifs.read((char*)&m_allIndexCount, sizeof(uint32_t));
	m_vertexArray.resize(m_allVertexCount);
	m_indexArray.resize(m_allIndexCount);
	ifs.read((char*)m_vertexArray.data(), sizeof(Vertex) * m_allVertexCount);
	ifs.read((char*)m_indexArray.data(), sizeof(uint32_t) * m_allIndexCount);


	m_vertexBuffer.create(device, queue, bufferCount, (UINT)sizeof(Vertex), (UINT)(sizeof(Vertex) * m_vertexArray.size()), m_vertexArray.data());
	m_indexBuffer.create(device, queue, bufferCount, (UINT)(sizeof(uint32_t) * m_indexArray.size()), m_indexArray.data());

	m_vertexCount.resize(m_materialCount);
	m_indexCount.resize(m_materialCount);
	m_albedoImageIndex.resize(m_materialCount);
	m_normalImageIndex.resize(m_materialCount);
	m_roughMetalImageIndex.resize(m_materialCount);
	for (uint32_t i = 0; i < m_materialCount; i++) {
		ifs.read((char*)&m_vertexCount[i], sizeof(uint32_t));
		ifs.read((char*)&m_indexCount[i], sizeof(uint32_t));
		ifs.read((char*)&m_albedoImageIndex[i], sizeof(uint32_t));
		ifs.read((char*)&m_normalImageIndex[i], sizeof(uint32_t));
		ifs.read((char*)&m_roughMetalImageIndex[i], sizeof(uint32_t));
	}

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
		if (!m_images[i].createResource(device, queue, bufferCount, m_imageWidths[i], m_imageHeights[i], 4, m_imageDatas[i].data(), true))
			abort();

		vector<unsigned char>().swap(m_imageDatas[i]);
	}

	ifs.close();

	return true;
}

void Mesh::saveModelCache() {
	
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
	ofs.write((const char*)m_vertexArray.data(), sizeof(Vertex) * m_allVertexCount);
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

	ofs.close();
	txt.close();
}

