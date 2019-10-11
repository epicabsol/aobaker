#pragma once

#include <d3d11.h>
#include <string>

class RenderShader
{
public:
	RenderShader(ID3D11InputLayout* inputLayout, ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader);
	~RenderShader();

	ID3D11InputLayout* GetInputLayout() const;
	ID3D11VertexShader* GetVertexShader() const;
	ID3D11PixelShader* GetPixelShader() const;

	static RenderShader* Create(const D3D11_INPUT_ELEMENT_DESC* inputElements, size_t inputElementCount, const std::wstring& vertexShaderFilename, const std::wstring& pixelShaderFilename);

private:
	ID3D11InputLayout* InputLayout;
	ID3D11VertexShader* VertexShader;
	ID3D11PixelShader* PixelShader;
};