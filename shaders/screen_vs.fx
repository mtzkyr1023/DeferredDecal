

struct VS_OUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

VS_OUT main(uint vertexId : SV_VertexID) {
	VS_OUT output = (VS_OUT)0;
	const float2 vId = float2((float)(vertexId / 2), (float)(vertexId % 2));
	
	output.pos = float4(vId.x * 2 - 1, vId.y * 2 - 1, 0.0f, 1.0f);
	output.tex = float2(vId.x, 1.0f - vId.y);
	
	return output;
}