Texture2D albedo	: register(t0);
Texture2D normal	: register(t1);
Texture2D roughMet	: register(t2);

SamplerState staticSampler : register(s0);

const float PI = 3.14159265359;

float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
	return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(1.0f - cosTheta, 5.0f);
}

struct VertexShaderOutput
{
	float4   Position		: SV_Position;
	float3   WorldPos		: WORLDPOS;
	float2	 TexCoords		: TEXCOORD0;
	float3	 Normal			: NORMAL;
	float3x3 TBN			: TBNMATRIX;
	float3   CameraPos		: CAMPOS;
};

struct Light
{
	float4 lightPos;
	float4 lightDir;
	float4 lightCol;
	float lightIntensity;
	float3 padding;
};

float4 main(VertexShaderOutput IN) : SV_Target
{
	float4 sampledAlbedo = albedo.Sample(staticSampler, IN.TexCoords);
	float3 albedo = pow(sampledAlbedo.rbb, 2.2f);
	float4 sampledNormal = normal.Sample(staticSampler, IN.TexCoords);
	//sampledNormal = normal.Sample(staticSampler, float2(IN.TexCoords.x, 1.0f - IN.TexCoords.y));

	float4 sampledMetRough = roughMet.Sample(staticSampler, IN.TexCoords);
	float metallic = sampledMetRough.b;
	float roughness = sampledMetRough.g;

	// Expand the range of the normal value from (0, +1) to (-1, +1).
	sampledNormal = (sampledNormal * 2.0f) - 1.0f;

	// Calculate the normal from the data in the bump map.
	float3 normal = mul(sampledNormal.xyz, IN.TBN);

	Light lights[4];

	lights[0].lightPos = float4(15.0f, 15.0f, -30.0f, 1.0f);
	lights[0].lightDir = float4(-normalize(lights[0].lightPos.xyz), 0.0f);
	lights[0].lightCol = float4(150.0f, 0.0f, 0.0f, 1.0f);
	lights[0].lightIntensity = 1.0f;

	lights[1].lightPos = float4(-15.0f, 15.0f, -30.0f, 1.0f);
	lights[1].lightDir = float4(-normalize(lights[1].lightPos.xyz), 0.0f);
	lights[1].lightCol = float4(0.0f, 150.0f, 0.0f, 1.0f);
	lights[1].lightIntensity = 1.0f;

	lights[2].lightPos = float4(15.0f, -15.0f, -30.0f, 1.0f);
	lights[2].lightDir = float4(-normalize(lights[2].lightPos.xyz), 0.0f);
	lights[2].lightCol = float4(0.0f, 0.0f, 150.0f, 1.0f);
	lights[2].lightIntensity = 1.0f;

	lights[3].lightPos = float4(-15.0f, -15.0f, -30.0f, 1.0f);
	lights[3].lightDir = float4(-normalize(lights[3].lightPos.xyz), 0.0f);
	lights[3].lightCol = float4(150.0f, 150.0f, 150.0f, 1.0f);
	lights[3].lightIntensity = 1.0f;

	float3 N = normalize(normal);
	float3 V = normalize(IN.CameraPos - IN.WorldPos);

	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	F0 = lerp(F0, albedo, metallic);

	float3 Lo = float3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < 4; ++i)
	{
		//float3 L = lights[i].lightDir;
		float3 L = normalize(lights[i].lightPos - IN.WorldPos);
		float3 H = normalize(V + L);

		float distance = length(lights[i].lightPos - IN.WorldPos);
		float attenutation = 1.0f / (distance * distance);

		float3 radiance = lights[i].lightCol * attenutation;

		float NDF = DistributionGGX(N, H, roughness);
		float G = GeometrySmith(N, V, L, roughness);
		float3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);

		float3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
		float3 dir_specular = numerator / max(denominator, 0.001f);

		float3 kS = F;
		float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
		kD *= 1.0f - metallic;

		float NdotL = max(dot(N, L), 0.0f);
		Lo += (kD * albedo / PI + dir_specular) * radiance * NdotL;
	}

	float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo;
	float3 color = ambient + Lo;

	color = color / (color + float3(1.0f, 1.0f, 1.0f));
	color = pow(color, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

	return float4(color, 1.0f);
}