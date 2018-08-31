#pragma once

#include <d3d12.h>
#include "RenderShader.h"

class RenderMaterial
{
public:
	ID3D12Resource1** GetTextures() const;
	int GetTextureCount() const;
	RenderShader* GetShader() const;

	void SetTexture(ID3D12Resource1* const texture, const int& slot);

	RenderMaterial(RenderShader* const shader, ID3D12Resource1** const textures, const int& textureCount);

private:
	ID3D12Resource1** Textures = nullptr;
	int TextureCount = 0;
	RenderShader* Shader = nullptr;
};