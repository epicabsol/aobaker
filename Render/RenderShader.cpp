#include "RenderShader.h"
#include <d3dcompiler.h>
#include "../Renderer.h"

ID3D12RootSignature* RenderShader::GetSignature() const
{
	return this->Signature;
}

ID3DBlob* RenderShader::GetVertexShader() const
{
	return this->VertexShader;
}

ID3DBlob* RenderShader::GetPixelShader() const
{
	return this->PixelShader;
}

ID3D12PipelineState* RenderShader::GetPipelineState() const
{
	return this->PipelineState;
}

void RenderShader::Dispose()
{
	this->Signature->Release();
	this->Signature = nullptr;
	this->VertexShader->Release();
	this->VertexShader = nullptr;
	this->PixelShader->Release();
	this->PixelShader = nullptr;
	this->PipelineState->Release();
	this->PipelineState = nullptr;
}

RenderShader* RenderShader::Create(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& rootSignatureDescription, const std::wstring& vertexShaderFilename, const std::wstring& pixelShaderFilename, const D3D12_INPUT_ELEMENT_DESC* inputElements, const unsigned int& inputElementCount)
{
	RenderShader* result = new RenderShader();
	HRESULT hr;

	// Pixel Shader
	result->PixelShader = Renderer::ReadBlobFromFile(pixelShaderFilename);
	if (result->PixelShader == nullptr)
	{
		delete result;
		return nullptr;
	}

	// Vertex Shader
	result->VertexShader = Renderer::ReadBlobFromFile(vertexShaderFilename);
	if (result->VertexShader == nullptr)
	{
		delete result;
		return nullptr;
	}

	// Root Signature
	ID3DBlob* rootSignatureData = nullptr;
	ID3DBlob* errorData = nullptr;
	hr = D3D12SerializeVersionedRootSignature(&rootSignatureDescription, &rootSignatureData, &errorData);
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to serialize the root signature.");
		delete result;
		return nullptr;
	}

	hr = Renderer::Device->CreateRootSignature(0, rootSignatureData->GetBufferPointer(), rootSignatureData->GetBufferSize(), IID_PPV_ARGS(&result->Signature));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create root signature from serialized form.");
		delete result;
		return nullptr;
	}

	// Pipeline State
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { };
	psoDesc.InputLayout = { inputElements, inputElementCount };
	psoDesc.pRootSignature = result->GetSignature();
	psoDesc.VS = { result->GetVertexShader()->GetBufferPointer(), result->GetVertexShader()->GetBufferSize() };
	psoDesc.PS = { result->GetPixelShader()->GetBufferPointer(), result->GetPixelShader()->GetBufferSize() };
	psoDesc.RasterizerState = { D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, FALSE, 0, 0.0f, 0.0f, FALSE, FALSE, FALSE, 0, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF };
	psoDesc.BlendState = { FALSE, FALSE };
	psoDesc.BlendState.RenderTarget[0] = { TRUE, FALSE, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD, D3D12_BLEND_INV_DEST_ALPHA, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD, D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.SampleMask = 0xFFFFFFFFu;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	hr = Renderer::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&result->PipelineState));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create graphics pipeline state.");
		delete result;
		return nullptr;
	}

	return result;
}

RenderShader::RenderShader()
{

}