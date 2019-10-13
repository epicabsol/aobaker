#pragma pack_matrix(row_major)

struct VS_IN
{
	float3 pos : POSITION;
	float3 norm : NORMAL;
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
};

struct VS_OUT
{
	float4 pos : SV_POSITION;
	float3 norm : TEXCOORD0;
	float2 uv : TEXCOORD1;
	float3 worldpos : TEXCOORD2;
};

struct ObjectConstants
{
	float4x4 model;
};

ObjectConstants Constants : register(b0);

VS_OUT main(VS_IN input)
{
	VS_OUT result;
	result.pos = float4(input.uv.x * 2.0f - 1.0f, 1.0f - input.uv.y * 2.0f, 0.0f, 1.0f);
	result.norm = mul(float4(input.norm, 0.0f), Constants.model).xyz;
	result.uv = input.uv;
	result.worldpos = mul(float4(input.pos, 1.0f), Constants.model).xyz;

	return result;
}