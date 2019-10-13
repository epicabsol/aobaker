#pragma once

class Scene;

namespace BakeEngine
{
	void Init();
	void Dispose();
	void Bake(Scene* scene);
}