struct ModelViewProjection
{
	float4x4 Model;
    float4x4 View;
	float4x4 Projection;
	float4	 CamPos;
};

ConstantBuffer<ModelViewProjection> MVP_CB : register(b0);

struct VertexPosColor
{
    float3 Position		: POSITION;
	float2 TexCoords	: TEXCOORD0;
	float3 Normal		: NORMAL;
	float3 Tangent		: TANGENT;
	float3 BiTangent	: BITANGENT;
};

struct VertexShaderOutput
{
	float4   Position		: SV_Position;
	float3   WorldPos		: WORLDPOS;
	float2	 TexCoords		: TEXCOORD0;
	float3	 Normal			: NORMAL;
	float3x3 TBN			: TBNMATRIX;
	float3   CameraPos		: CAMPOS;
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

	float4 pos = float4(IN.Position, 1.0f);

	float4 worldPos = mul(MVP_CB.Model, pos);
	float4 viewPos = mul(MVP_CB.View, worldPos);
	float4 viewProjPos = mul(MVP_CB.Projection, viewPos);

	OUT.WorldPos = worldPos;
	OUT.Position = viewProjPos;
	OUT.TexCoords = IN.TexCoords;

	OUT.CameraPos = MVP_CB.CamPos.xyz;

	OUT.Normal = mul(MVP_CB.Model, float4(IN.Normal, 0.0f)).xyz;

	float3 tangent = mul(MVP_CB.Model, float4(IN.Tangent, 0.0f)).xyz;
	float3 bitangent = mul(MVP_CB.Model, float4(IN.BiTangent, 0.0f)).xyz;

	OUT.TBN = float3x3(tangent, bitangent, OUT.Normal);

    return OUT;
}