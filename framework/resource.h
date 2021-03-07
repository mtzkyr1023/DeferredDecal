#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#include <d3d12.h>

#include <wrl/client.h>
#include <vector>

class Resource {
public:
	virtual ~Resource() = default;

	ID3D12Resource* getResource(UINT num) { return m_resource[num].Get(); }

protected:
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_resource;

};

#endif