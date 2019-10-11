#pragma once

#include "../Render/RenderTexture.h"

class BakeTexture {
public:
	BakeTexture(const int& width, const int& height, const int& channelCount, const int& bytesPerChannel);
	~BakeTexture();
	unsigned char* GetData() const;
	size_t GetDataLength() const;
	RenderTexture* GetGPUPreview() const;
	void UpdateData(unsigned char* data, size_t length);
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