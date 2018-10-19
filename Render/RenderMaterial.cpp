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
	this->Textures = textures;
	this->TextureCount = textureCount;
}