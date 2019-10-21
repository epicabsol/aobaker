#pragma once

#include "../Render/RenderTexture.h"

class BakeTexture {
public:
	BakeTexture(const int& width, const int& height, const int& channelCount, const int& bytesPerChannel);
	~BakeTexture();
	inline int GetWidth() const { return this->Width; }
	inline int GetHeight() const { return this->Height; }
	inline int GetChannelCount() const { return this->ChannelCount; }
	inline int GetBytesPerChannel() const { return this->BytesPerChannel; }
	unsigned char* GetData() const;
	size_t GetDataLength() const;
	RenderTexture* GetGPUPreview() const;
	void UpdateData(unsigned char* data, size_t length);
	void SavePNG(const wchar_t* filename) const;
	void RefreshPreview() const;
	void FillRandom();

private:
	int Width;
	int Height;
	int ChannelCount;
	int BytesPerChannel;
	DXGI_FORMAT PreviewFormat;

	unsigned char* Data;
	size_t DataLength;
	RenderTexture* GPUPreview;
};