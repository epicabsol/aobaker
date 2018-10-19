#pragma once

#include <d3d12.h>
#include "RenderShader.h"
#include "RenderTexture.h"

class RenderMaterial
{
public:
	RenderTexture** GetTextures() const;
	int GetTextureCount() const;
	RenderShader* GetShader() const;

	void SetTexture(RenderTexture* const texture, const int& slot);

	RenderMaterial(RenderShader* const shader, RenderTexture** const textures, const int& textureCount);
	~RenderMaterial();

private:
	RenderTexture** Textures = nullptr;
	int TextureCount = 0;
	RenderShader* Shader = nullptr;
};