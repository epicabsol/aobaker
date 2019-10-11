#pragma once

#include <d3d11.h>

class RenderTexture 
{
public:
	RenderTexture(const int& width, const int& height, const DXGI_FORMAT& format, ID3D11Texture2D* gpuTexture, ID3D11ShaderResourceView* srv, ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv);
	~RenderTexture();

	int GetWidth() const;
	int GetHeight() const;
	DXGI_FORMAT GetFormat() const;
	ID3D11Texture2D* GetGPUTexture() const;
	ID3D11ShaderResourceView* GetSRV() const;
	ID3D11RenderTargetView* GetRTV() const;
	ID3D11DepthStencilView* GetDSV() const;

	void UploadData(ID3D11DeviceContext* context, const unsigned char* data, const size_t& length) const;
	void UploadCheckerboard(ID3D11DeviceContext* context, char r, char g, char b) const;

	static RenderTexture* Create(const int& width, const int& height, const DXGI_FORMAT& format, const unsigned char* initialData, const size_t initialDataLength, bool dynamic);

private:
	int Width;
	int Height;
	DXGI_FORMAT Format;
	ID3D11Texture2D* GPUTexture;
	ID3D11ShaderResourceView* SRV;
	ID3D11RenderTargetView* RTV;
	ID3D11DepthStencilView* DSV;
};