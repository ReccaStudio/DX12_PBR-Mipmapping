struct VertexOutputType
{
	float4 position : SV_Position;
	float2 tex : TEXCOORD0;
};

VertexOutputType main(uint VertexID: SV_VertexID)
{
	VertexOutputType output;

	float2 texcoord = float2(VertexID >> 1, VertexID & 1);
	float4 position = float4((texcoord.x - 0.5f) * 2.0f, (texcoord.y - 0.5f) * 2.0f, 0.0f, 1.0f);

	output.position = position;
	output.tex = texcoord;

	return output;
}