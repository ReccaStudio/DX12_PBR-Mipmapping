struct ViewProjection
{
    matrix VP;
};

ConstantBuffer<ViewProjection> ViewProjectionCB : register(b0);

struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Color    : COLOR;
	matrix WorldMat : WORLDMAT;
};

struct VertexShaderOutput
{
	float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

	matrix MVP = mul(ViewProjectionCB.VP, IN.WorldMat);

    OUT.Position = mul(MVP, float4(IN.Position, 1.0f));
    OUT.Color = float4(IN.Color, 1.0f);

    return OUT;
}