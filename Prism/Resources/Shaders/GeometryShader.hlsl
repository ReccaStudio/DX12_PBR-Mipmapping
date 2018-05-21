struct VertexShaderOutput
{
	float4 Color    : COLOR;
	float4 Position : POSITION;
	matrix WorldMat : WORLD;
	matrix ViewProj : VIEW_PROJ;
};


struct PixelShaderInput
{
	float4 Color    : COLOR;
	float4 Position : SV_Position;
};

[maxvertexcount(3)]
void main(triangle VertexShaderOutput input[3], inout TriangleStream<PixelShaderInput> OutputStream)
{
	PixelShaderInput output;

	float3 edge1 = normalize((input[2].Position - input[0].Position).xyz);
	float3 edge2 = normalize((input[1].Position - input[0].Position).xyz);

	float3 faceNormal = cross(edge1, edge2);

	for (uint i = 0; i < 3; i++)
	{
		output.Color = input[i].Color;
		output.Position = input[i].Position + float4(faceNormal, 0.0f);

		matrix MVP = mul(input[i].ViewProj, input[i].WorldMat);

		output.Position = mul(MVP, output.Position);

		OutputStream.Append(output);
	}

	OutputStream.RestartStrip();
} 