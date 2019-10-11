#include "RenderTexture.h"
#include "../Renderer.h"

RenderTexture::RenderTexture(const int& width, const int& height, const DXGI_FORMAT& format, ID3D11Texture2D* gpuTexture, ID3D11ShaderResourceView* srv, ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv)
	: Width(width), Height(height), Format(format), GPUTexture(gpuTexture), SRV(srv), RTV(rtv), DSV(dsv)
{
	if (width <= 0 || height <= 0)
		throw L"Invalid size given to RenderTexture::RenderTexture.";
}

RenderTexture::~RenderTexture()
{
	if (this->DSV != nullptr)
	{
		this->DSV->Release();
		this->DSV = nullptr;
	}
	if (this->RTV != nullptr)
	{
		this->RTV->Release();
		this->RTV = nullptr;
	}
	if (this->SRV != nullptr)
	{
		this->SRV->Release();
		this->SRV = nullptr;
	}
	if (this->GPUTexture != nullptr)
	{
		this->GPUTexture->Release();
		this->GPUTexture = nullptr;
	}
}

int RenderTexture::GetWidth() const 
{
	return this->Width;
}

int RenderTexture::GetHeight() const 
{
	return this->Height;
}

DXGI_FORMAT RenderTexture::GetFormat() const 
{
	return this->Format;
}

ID3D11Texture2D* RenderTexture::GetGPUTexture() const
{
	return this->GPUTexture;
}

ID3D11ShaderResourceView* RenderTexture::GetSRV() const
{
	return this->SRV;
}

ID3D11RenderTargetView* RenderTexture::GetRTV() const
{
	return this->RTV;
}

ID3D11DepthStencilView* RenderTexture::GetDSV() const
{
	return this->DSV;
}

void RenderTexture::UploadData(ID3D11DeviceContext* context, const unsigned char* data, const size_t& length) const
{
	D3D11_MAPPED_SUBRESOURCE subresource = { };
	HRESULT hr = context->Map(this->GPUTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to map texture for upload!\n");
		return;
	}

	memcpy(subresource.pData, data, length);

	context->Unmap(this->GPUTexture, 0);
}

void RenderTexture::UploadCheckerboard(ID3D11DeviceContext* context, char r, char g, char b) const
{
	static unsigned char data[64 * 64 * 4];
	for (int y = 0; y < 64; y++)
	{
		for (int x = 0; x < 64; x++)
		{
			data[y * 64 * 4 + x * 4 + 0] = (x / 16 + y / 16) % 2 == 0 ? b : 0; // B
			data[y * 64 * 4 + x * 4 + 1] = (x / 16 + y / 16) % 2 == 0 ? g : 0; // G
			data[y * 64 * 4 + x * 4 + 2] = (x / 16 + y / 16) % 2 == 0 ? r : 0; // R
			data[y * 64 * 4 + x * 4 + 3] = 255; // A
		}
	}
	this->UploadData(context, data, (size_t)(64 * 64 * 4));
}

RenderTexture* RenderTexture::Create(const int& width, const int& height, const DXGI_FORMAT& format, const unsigned char* initialData, const size_t initialDataLength, bool dynamic)
{
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* srv = nullptr;

	D3D11_TEXTURE2D_DESC texdesc = { };
	texdesc.ArraySize = 1;
	texdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texdesc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
	texdesc.Format = format;
	texdesc.Width = width;
	texdesc.Height = height;
	texdesc.MipLevels = 1;
	texdesc.SampleDesc.Count = 1;
	texdesc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
	D3D11_SUBRESOURCE_DATA initialResData = { };
	initialResData.pSysMem = initialData;
	initialResData.SysMemPitch = initialDataLength / height;
	HRESULT hr = Renderer::Device->CreateTexture2D(&texdesc, initialData == nullptr ? nullptr : &initialResData, &texture);
	if (FAILED(hr))
	{
		throw "o no";
	}

	hr = Renderer::Device->CreateShaderResourceView(texture, nullptr, &srv);
	if (FAILED(hr))
	{
		throw "o no";
	}

	return new RenderTexture(width, height, format, texture, srv, nullptr, nullptr);
}