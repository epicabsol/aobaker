#include "BakeTexture.h"

#include <stdint.h>

BakeTexture::BakeTexture(const int& width, const int& height, const int& channelCount, const int& bytesPerChannel)
{
	// Valid combinations of ChannelCount and BytesPerChannel:
	// 1, 2, 4 channels of 1 byte, which will be UNORM
	// 1, 2, 4 channels of 2 bytes, which will be FLOAT
	// 1, 2, 3, 4 channels of 4 bytes, which will be FLOAT
	if (bytesPerChannel == 1)
	{
		if (channelCount == 1)
			this->PreviewFormat = DXGI_FORMAT_R8_UNORM;
		else if (channelCount == 2)
			this->PreviewFormat = DXGI_FORMAT_R8G8_UNORM;
		else if (channelCount == 4)
			this->PreviewFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		else
			this->PreviewFormat = DXGI_FORMAT_UNKNOWN;
	}
	else if (bytesPerChannel == 2)
	{
		if (channelCount == 1)
			this->PreviewFormat = DXGI_FORMAT_R16_FLOAT;
		else if (channelCount == 2)
			this->PreviewFormat = DXGI_FORMAT_R16G16_FLOAT;
		else if (channelCount == 4)
			this->PreviewFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		else
			this->PreviewFormat = DXGI_FORMAT_UNKNOWN;
	}
	else if (bytesPerChannel == 4)
	{
		if (channelCount == 1)
			this->PreviewFormat = DXGI_FORMAT_R32_FLOAT;
		else if (channelCount == 2)
			this->PreviewFormat = DXGI_FORMAT_R32G32_FLOAT;
		else if (channelCount == 3)
			this->PreviewFormat == DXGI_FORMAT_R32G32B32_FLOAT;
		else if (channelCount == 4)
			this->PreviewFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		else
			this->PreviewFormat = DXGI_FORMAT_UNKNOWN;
	}
	else
		this->PreviewFormat = DXGI_FORMAT_UNKNOWN;

	if (this->PreviewFormat == DXGI_FORMAT_UNKNOWN)
	{
		OutputDebugStringW(L"Invalid combination of channelCount and bytesPerChannel.");
		DebugBreak();
	}

	this->Width = width;
	this->Height = height;
	this->ChannelCount = channelCount;
	this->BytesPerChannel = bytesPerChannel;
	this->DataLength = width * height * channelCount * bytesPerChannel;
	this->Data = new char[this->DataLength];
	memset(this->Data, 0xFF, this->DataLength);
	this->GPUPreview = new RenderTexture(this->Width, this->Height, this->PreviewFormat);
	this->RefreshPreview();
	//this->FillRandom();
}

BakeTexture::~BakeTexture()
{
	delete[] this->Data;
	this->Data = nullptr;
	delete this->GPUPreview;
	this->GPUPreview = nullptr;
}

char* BakeTexture::GetData() const
{
	return this->Data;
}

size_t BakeTexture::GetDataLength() const
{
	return this->DataLength;
}

RenderTexture* BakeTexture::GetGPUPreview() const
{
	return this->GPUPreview;
}

void BakeTexture::UpdateData(char* data, size_t length)
{
	if (length != this->DataLength)
	{
		DebugBreak();
	}

	memcpy(this->Data, data, length);

	this->RefreshPreview();
}

void BakeTexture::RefreshPreview() const
{
	this->GPUPreview->UploadData(this->Data, this->DataLength);
}

void BakeTexture::FillRandom()
{
	for (int y = 0; y < this->Height; y++)
	{
		for (int x = 0; x < this->Width; x++)
		{
			for (int channel = 0; channel < this->ChannelCount; channel++)
			{
				size_t channelStart = channel * this->BytesPerChannel + this->ChannelCount * this->BytesPerChannel * x + this->ChannelCount * this->BytesPerChannel * this->Width * y;
				if (this->BytesPerChannel == 1)
				{
					Data[channelStart] = 0xFF * rand() / RAND_MAX;
				}
				else if (this->BytesPerChannel == 2)
				{
					uint16_t* value = (uint16_t*)&Data[channelStart];
					*value = 0xFFFF * rand() / RAND_MAX;
				}
				else if (this->BytesPerChannel == 4)
				{
					uint32_t* value = (uint32_t*)&Data[channelStart];
					*value = 0xFFFFFFFF * rand() / RAND_MAX;
				}
			}
		}
	}
	this->RefreshPreview();
}