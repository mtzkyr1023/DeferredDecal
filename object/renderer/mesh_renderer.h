#ifndef _MESH_RENDERER_H_
#define _MESH_RENDERER_H_

#include <string>

class MeshRenderer {
public:
	MeshRenderer();
	MeshRenderer(const char* filename);
	~MeshRenderer();

	void setFilename(const char* filename);
	const std::string& filename() { return m_filename; }

private:
	std::string m_filename;
};


#endif