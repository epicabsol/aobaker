#pragma once

#include "../Render/RenderTexture.h"

class BakeTexture {
public:
	BakeTexture(const int& width, const int& height, const int& channelCount, const int& bytesPerChannel);
	~BakeTexture();
	char* GetData() const;
	size_t GetDataLength() const;
	RenderTexture* GetGPUPreview() const;
	void UpdateData(char* data, size_t length);

private:
	int Width;
	int Height;
	int ChannelCount;
	int BytesPerChannel;

	char* Data;
	size_t DataLength;
	RenderTexture* GPUPreview;
};