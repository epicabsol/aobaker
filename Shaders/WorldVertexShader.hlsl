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
	float4 color : COLOR0;
};

struct WorldConstants
{
	float4x4 model;
	float4x4 view;
	float4x4 projection;
};

ConstantBuffer<WorldConstants> Constants : register(b0);

VS_OUT main(VS_IN input)
{
	VS_OUT result;
	result.pos = mul(mul(mul(float4(input.pos, 1.0f), Constants.model), Constants.view), Constants.projection); // TODO: Multiply input.pos by model, view & projection matrices
	result.norm = input.norm; // TODO: Multiply by model matrix
	result.uv = input.uv;
	result.color = input.color;

	return result;
}