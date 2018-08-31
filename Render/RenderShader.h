#pragma once

#include <d3d12.h>
#include <string>

class RenderShader
{
public:
	ID3D12RootSignature* GetSignature() const;
	ID3DBlob* GetVertexShader() const;
	ID3DBlob* GetPixelShader() const;
	ID3D12PipelineState* GetPipelineState() const;
	void Dispose();

	static RenderShader* Create(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& rootSignatureDescription, const std::wstring& vertexShaderFilename, const std::wstring& pixelShaderFilename, const D3D12_INPUT_ELEMENT_DESC* inputElements, const unsigned int& inputElementCount);

private:
	ID3D12RootSignature* Signature = nullptr;
	ID3DBlob* VertexShader = nullptr;
	ID3DBlob* PixelShader = nullptr;
	ID3D12PipelineState* PipelineState = nullptr;
	RenderShader();
};