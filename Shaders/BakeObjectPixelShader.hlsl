#pragma pack_matrix(row_major)

struct PS_IN
{
	float4 pos : SV_POSITION;
	float3 norm : TEXCOORD0;
	float2 uv : TEXCOORD1;
	float4 color : COLOR0;
};

Texture2D AOTexture : register(t0);
SamplerState Sampler {

};

float4 main(PS_IN input) : SV_TARGET
{
	float value = pow(AOTexture.Sample(Sampler, input.uv).r, 1.0f / 2.2f);
	return float4(value, value, value, 1.0f);
	//return float4(input.norm, 1.0f);
}