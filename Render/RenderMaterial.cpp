#include "RenderMaterial.h"

RenderTexture** RenderMaterial::GetTextures() const
{
	return this->Textures;
}

int RenderMaterial::GetTextureCount() const
{
	return this->TextureCount;
}

RenderShader* RenderMaterial::GetShader() const
{
	return this->Shader;
}

void RenderMaterial::SetTexture(RenderTexture* const texture, const int& slot)
{
	this->Textures[slot] = texture;
}

RenderMaterial::RenderMaterial(RenderShader* const shader, RenderTexture** const textures, const int& textureCount)
{
	this->Shader = shader;
	this->TextureCount = textureCount;
	this->Textures = new RenderTexture*[this->TextureCount];
	for (size_t i = 0; i < this->TextureCount; i++)
		this->Textures[i] = textures[i];
}

RenderMaterial::~RenderMaterial()
{
	delete[] this->Textures;
}