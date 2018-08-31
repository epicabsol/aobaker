#include "RenderMaterial.h"

ID3D12Resource1** RenderMaterial::GetTextures() const
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

void RenderMaterial::SetTexture(ID3D12Resource1* const texture, const int& slot)
{
	this->Textures[slot] = texture;
}

RenderMaterial::RenderMaterial(RenderShader* const shader, ID3D12Resource1** const textures, const int& textureCount)
{
	this->Shader = shader;
	this->Textures = textures;
	this->TextureCount = textureCount;
}