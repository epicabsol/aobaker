#pragma once

#include "Scene/Scene.h"
#include "Render/RenderShader.h"

Scene* GetCurrentScene();
RenderShader* GetWorldShader();
RenderShader* GetBakeObjectShader();
RenderMaterial* GetTestMaterial();
RenderMesh* GetTestMesh();
void Initialize();
void Update(const float& dT);
void Render();
void MouseDown(const int& button, const int& x, const int& y);
void MouseMove(const int& x, const int& y);
void MouseUp(const int& button, const int& x, const int& y);
void KeyDown(const int& vkey);
void KeyUp(const int& vkey);
void Dispose();
