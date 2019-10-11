#include "RenderShader.h"
#include <d3dcompiler.h>
#include "../Renderer.h"

RenderShader::RenderShader(ID3D11InputLayout* inputLayout, ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader) : InputLayout(inputLayout), VertexShader(vertexShader), PixelShader(pixelShader)
{

}

RenderShader::~RenderShader() {
	if (this->VertexShader != nullptr)
	{
		this->VertexShader->Release();
		this->VertexShader = nullptr;
	}
	if (this->PixelShader != nullptr)
	{
		this->PixelShader->Release();
		this->PixelShader = nullptr;
	}
}

ID3D11InputLayout* RenderShader::GetInputLayout() const
{
	return this->InputLayout;
}

ID3D11VertexShader* RenderShader::GetVertexShader() const
{
	return this->VertexShader;
}

ID3D11PixelShader* RenderShader::GetPixelShader() const
{
	return this->PixelShader;
}

RenderShader* RenderShader::Create(const D3D11_INPUT_ELEMENT_DESC* inputElements, size_t inputElementCount, const std::wstring& vertexShaderFilename, const std::wstring& pixelShaderFilename)
{
	ID3D11InputLayout* inputLayout = nullptr;
	ID3D11VertexShader* vertexShader = nullptr;
	ID3D11PixelShader* pixelShader = nullptr;

	ID3DBlob* vsBlob = Renderer::ReadBlobFromFile(vertexShaderFilename);
	ID3DBlob* psBlob = Renderer::ReadBlobFromFile(pixelShaderFilename);

	HRESULT hr = Renderer::Device->CreateInputLayout(inputElements, inputElementCount, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
	hr = Renderer::Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
	hr = Renderer::Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);

	return new RenderShader(inputLayout, vertexShader, pixelShader);
}