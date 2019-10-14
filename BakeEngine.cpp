#include "BakeEngine.h"

#include "Renderer.h"

#include "Render/RenderTexture.h"

#include "Scene/Scene.h"
#include "Render/ConstantBuffer.h"
#include "Scene/BakeTexture.h"

#include "RadeonRays/radeon_rays.h"

const int sampleCount = 1000;
const float falloffDistance = 1.25f;
const bool randomizeRays = false;
const int rayChunkSize = 0xFFFFFF;

ID3D11DepthStencilState* LayerDSS = nullptr;
ID3D11RasterizerState* NoCullRS = nullptr;

struct GenerateRaysOptions
{
	int SampleCount;
	DirectX::SimpleMath::Vector3 Unused;
};

const D3D11_INPUT_ELEMENT_DESC RasterizeInputElements[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0}
};
const std::wstring RasterizeVertexShaderFilename = L"data/RasterizeVertexShader.cso";
const std::wstring RasterizePixelShaderFilename = L"data/RasterizePixelShader.cso";
const std::wstring GenerateRaysComputeShaderFilename = L"data/GenerateRaysComputeShader.cso";
RenderShader* RasterizeShader = nullptr;
ID3D11ComputeShader* GenerateRaysComputeShader = nullptr;
ConstantBuffer<GenerateRaysOptions>* GenerateRaysConstantBuffer = nullptr;

ID3D11Buffer* TexelBuffer = nullptr;
ID3D11ShaderResourceView* TexelBufferSRV = nullptr;
ID3D11Buffer* HemisphereBuffer = nullptr;
ID3D11ShaderResourceView* HemisphereBufferSRV = nullptr;
ID3D11Buffer* RayOutputBuffer = nullptr;
ID3D11UnorderedAccessView* RayOutputBufferUAV = nullptr;
ID3D11Buffer* RayOutputStagingBuffer = nullptr;

struct ObjectConstants
{
	DirectX::SimpleMath::Matrix Model;
};
ConstantBuffer<ObjectConstants>* ObjectConstantBuffer = nullptr;

struct TexelRasterization
{
	int TexelX;
	int TexelY;
	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector2 TexCoord;
	DirectX::SimpleMath::Vector3 Normal;

	TexelRasterization() : TexelRasterization(-1, -1, DirectX::SimpleMath::Vector3(), DirectX::SimpleMath::Vector2(), DirectX::SimpleMath::Vector3(), 0)
	{

	}

	TexelRasterization(int texelX, int texelY, DirectX::SimpleMath::Vector3 position, DirectX::SimpleMath::Vector2 texCoord, DirectX::SimpleMath::Vector3 normal, uint32_t firstRayIndex)
		: TexelX(texelX), TexelY(texelY), Position(position), TexCoord(texCoord), Normal(normal)
	{

	}
};

void BakeEngine::Init()
{
	D3D11_DEPTH_STENCIL_DESC dsdesc = { };
	dsdesc.StencilEnable = true;
	dsdesc.DepthEnable = false;
	dsdesc.StencilReadMask = 0xFF;
	dsdesc.StencilWriteMask = 0xFF;
	dsdesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dsdesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsdesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_INCR_SAT;
	dsdesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR_SAT;
	dsdesc.BackFace = dsdesc.FrontFace;
	HRESULT hr = Renderer::Device->CreateDepthStencilState(&dsdesc, &LayerDSS);

	D3D11_RASTERIZER_DESC rsdesc = { };
	rsdesc.CullMode = D3D11_CULL_NONE;
	rsdesc.FillMode = D3D11_FILL_SOLID;
	hr = Renderer::Device->CreateRasterizerState(&rsdesc, &NoCullRS);

	RasterizeShader = RenderShader::Create(RasterizeInputElements, ARRAYSIZE(RasterizeInputElements), RasterizeVertexShaderFilename, RasterizePixelShaderFilename);

	ObjectConstantBuffer = ConstantBuffer<ObjectConstants>::Create(Renderer::Device, ObjectConstants{ DirectX::SimpleMath::Matrix() }, true);

	ID3DBlob* generateRaysBytecode = Renderer::ReadBlobFromFile(GenerateRaysComputeShaderFilename);
	hr = Renderer::Device->CreateComputeShader(generateRaysBytecode->GetBufferPointer(), generateRaysBytecode->GetBufferSize(), nullptr, &GenerateRaysComputeShader);
	generateRaysBytecode->Release();

	GenerateRaysConstantBuffer = ConstantBuffer<GenerateRaysOptions>::Create(Renderer::Device, { 0 }, true);

	D3D11_BUFFER_DESC bufdesc = { };
	D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc = { };
	// Texel buffer
	bufdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bufdesc.ByteWidth = rayChunkSize * sizeof(TexelRasterization); // Worst case is 1 texel per ray
	bufdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufdesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufdesc.StructureByteStride = sizeof(TexelRasterization);
	bufdesc.Usage = D3D11_USAGE_DYNAMIC;
	hr = Renderer::Device->CreateBuffer(&bufdesc, nullptr, &TexelBuffer);
	srvdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvdesc.Buffer.FirstElement = 0;
	srvdesc.Buffer.NumElements = rayChunkSize; // Worst case is 1 texel per ray
	srvdesc.Format = DXGI_FORMAT_UNKNOWN;
	hr = Renderer::Device->CreateShaderResourceView(TexelBuffer, &srvdesc, &TexelBufferSRV);
	// Hemisphere buffer
	bufdesc.ByteWidth = rayChunkSize * sizeof(DirectX::SimpleMath::Vector3);
	bufdesc.StructureByteStride = sizeof(DirectX::SimpleMath::Vector3);
	hr = Renderer::Device->CreateBuffer(&bufdesc, nullptr, &HemisphereBuffer);
	hr = Renderer::Device->CreateShaderResourceView(HemisphereBuffer, &srvdesc, &HemisphereBufferSRV);
	// Ray Output buffer
	bufdesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	bufdesc.ByteWidth = rayChunkSize * sizeof(RadeonRays::ray);
	bufdesc.CPUAccessFlags = 0;
	bufdesc.StructureByteStride = sizeof(RadeonRays::ray);
	bufdesc.Usage = D3D11_USAGE_DEFAULT;
	hr = Renderer::Device->CreateBuffer(&bufdesc, nullptr, &RayOutputBuffer);
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavdesc = { };
	uavdesc.Format = DXGI_FORMAT_UNKNOWN;
	uavdesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavdesc.Buffer.FirstElement = 0;
	uavdesc.Buffer.NumElements = rayChunkSize;
	hr = Renderer::Device->CreateUnorderedAccessView(RayOutputBuffer, &uavdesc, &RayOutputBufferUAV);
	// Ray Output staging buffer
	bufdesc.BindFlags = 0;
	bufdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	bufdesc.MiscFlags = 0;
	bufdesc.Usage = D3D11_USAGE_STAGING;
	hr = Renderer::Device->CreateBuffer(&bufdesc, nullptr, &RayOutputStagingBuffer);
}

void BakeEngine::Dispose()
{
	RayOutputStagingBuffer->Release();
	RayOutputStagingBuffer = nullptr;

	RayOutputBufferUAV->Release();
	RayOutputBufferUAV = nullptr;

	RayOutputBuffer->Release();
	RayOutputBuffer = nullptr;

	HemisphereBufferSRV->Release();
	HemisphereBufferSRV = nullptr;

	HemisphereBuffer->Release();
	HemisphereBuffer = nullptr;

	TexelBufferSRV->Release();
	TexelBufferSRV = nullptr;

	TexelBuffer->Release();
	TexelBuffer = nullptr;

	delete GenerateRaysConstantBuffer;
	GenerateRaysConstantBuffer = nullptr;

	GenerateRaysComputeShader->Release();
	GenerateRaysComputeShader = nullptr;

	delete ObjectConstantBuffer;
	ObjectConstantBuffer = nullptr;

	delete RasterizeShader;
	RasterizeShader = nullptr;

	NoCullRS->Release();
	NoCullRS = nullptr;

	LayerDSS->Release();
	LayerDSS = nullptr;
}

struct PositionUPixel
{
	DirectX::SimpleMath::Vector3 Position;
	float U;
};

struct NormalVPixel
{
	DirectX::PackedVector::XMHALF4 NormalV;
};

struct DepthStencilPixel
{
	uint32_t Depth : 24;
	uint32_t Stencil : 8;
};

struct BakeLayer
{
	TexelRasterization* Texels;
	int TexelCount;

	BakeLayer() : Texels(nullptr), TexelCount(0)
	{

	}

	BakeLayer(int texelCount) : Texels(nullptr), TexelCount(texelCount) {
		//this->Texels = new TexelRasterization[texelCount];
	}

	~BakeLayer() {
		/*if (this->Texels)
		{
			delete[] this->Texels;
			this->Texels = nullptr;
		}*/
	}
};

struct ResultValue
{
	float Total = 0.0f;
	float Samples = 0;

	void Add(float value, float weight) {
		this->Total += value * weight;
		this->Samples += weight;
	}

	float GetAverage() {
		if (this->Samples == 0)
			return 0.0f;
		return this->Total / this->Samples;
	}
};

struct TextureBakeData
{
	BakeTexture* BakeTexture;
	int Width;
	int Height;
	ResultValue* Values;
	BakeLayer* Layers;
	int LayerCount;

	TextureBakeData(class BakeTexture* bakeTexture, int width, int height) : BakeTexture(bakeTexture), Width(width), Height(height), Values(nullptr), Layers(nullptr), LayerCount(0)
	{
		Values = new ResultValue[Width * Height];
	}

	~TextureBakeData()
	{
		delete[] Values;
		delete[] Layers;
	}
};

// Algorithm from https://github.com/Lord-Nazdar/Sampling-Python/blob/master/UniformSampling.py#L100
void GenerateHemisphereSamples(DirectX::SimpleMath::Vector3 *buffer, int count) {
	for (int i = 0; i < count; i++) {
		// Sample two random values from 0-1
		float u = (float)std::rand() / RAND_MAX;
		float v = (float)std::rand() / RAND_MAX;

		float sintheta = std::sqrtf(-u * (u - 2.0f));
		float phi = 2.0f * PI * v;

		// Convert to cartesian
		float x = sintheta * std::cosf(phi);
		float z = sintheta * std::sinf(phi);

		// Compute Y
		float len = std::sqrtf(x * x + z * z);
		float y = 1.0f - len;
		//buffer[i] = DirectX::SimpleMath::Vector3(x, y, z); // 34%
		buffer[i].x = x;
		buffer[i].y = y;
		buffer[i].z = z;
	}
}

float DistanceToRadiance(float distance, float falloffDistance) {
	if (distance > falloffDistance)
		distance = falloffDistance;
	return powf(distance / falloffDistance, 2.0f);
}

void BakeEngine::Bake(Scene* scene)
{
#define COMPUTE_SHADER
	using namespace DirectX::SimpleMath;
	using namespace RadeonRays;

	std::vector<TextureBakeData*> textureBakeDatas;
	size_t sliceCount = 0;
	// HACK: Get frame to show on graphics analyzer
	//MessageBoxA(0, "Click the capture frame button!", "Dev", 0);
	Renderer::SwapChain->Present(1, 0);

	// Step 0: Upload the compute shader settings!
	GenerateRaysConstantBuffer->Update(Renderer::ImmediateContext, { sampleCount });

	HRESULT hr = S_OK;
	// Step 1: Rasterize into layers (Must be in sync on main thread)
	OutputDebugStringW(L"Rasterizing Geometry...\n");
	ID3D11Buffer* constantBuffer = ObjectConstantBuffer->GetBuffer();
	Renderer::ImmediateContext->VSSetConstantBuffers(0, 1, &constantBuffer);
	Renderer::ImmediateContext->IASetInputLayout(RasterizeShader->GetInputLayout());
	Renderer::ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Renderer::ImmediateContext->VSSetShader(RasterizeShader->GetVertexShader(), nullptr, 0);
	Renderer::ImmediateContext->PSSetShader(RasterizeShader->GetPixelShader(), nullptr, 0);
	Renderer::ImmediateContext->RSSetState(NoCullRS);
	std::vector<TexelRasterization> texels;
	for (int i = 0; i < scene->GetMaterialCount(); i++)
	{
		MaterialObject* material = scene->GetMaterial(i);

		ID3D11Texture2D* PositionUBuffer = nullptr; // R32G32B32A32 (X, Y, Z, U)
		ID3D11Texture2D* PositionUStaging = nullptr;
		ID3D11RenderTargetView* PositionUBufferRTV = nullptr;
		ID3D11Texture2D* NormalVBuffer = nullptr; // R16G16B16A16 (NX, NY, NZ, V)
		ID3D11Texture2D* NormalVStaging = nullptr;
		ID3D11RenderTargetView* NormalVBufferRTV = nullptr;
		ID3D11Texture2D* DepthStencilBuffer = nullptr; // D24S8 (Depth Unused, Stencil)
		ID3D11Texture2D* DepthStencilStaging = nullptr;
		ID3D11DepthStencilView* DepthStencilBufferDSV = nullptr;

		D3D11_TEXTURE2D_DESC texdesc = { };
		texdesc.ArraySize = 1;
		texdesc.BindFlags = D3D11_BIND_RENDER_TARGET;
		texdesc.CPUAccessFlags = 0;
		texdesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		texdesc.Height = material->GetAOTexture()->GetHeight();
		texdesc.Width = material->GetAOTexture()->GetWidth();
		texdesc.MipLevels = 1;
		texdesc.SampleDesc.Count = 1;
		texdesc.Usage = D3D11_USAGE_DEFAULT;

		hr = Renderer::Device->CreateTexture2D(&texdesc, nullptr, &PositionUBuffer);
		hr = Renderer::Device->CreateRenderTargetView(PositionUBuffer, nullptr, &PositionUBufferRTV);
		texdesc.Usage = D3D11_USAGE_STAGING;
		texdesc.BindFlags = 0;
		texdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		hr = Renderer::Device->CreateTexture2D(&texdesc, nullptr, &PositionUStaging);

		texdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texdesc.Usage = D3D11_USAGE_DEFAULT;
		texdesc.BindFlags = D3D11_BIND_RENDER_TARGET;
		texdesc.CPUAccessFlags = 0;
		hr = Renderer::Device->CreateTexture2D(&texdesc, nullptr, &NormalVBuffer);
		hr = Renderer::Device->CreateRenderTargetView(NormalVBuffer, nullptr, &NormalVBufferRTV);
		texdesc.Usage = D3D11_USAGE_STAGING;
		texdesc.BindFlags = 0;
		texdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		hr = Renderer::Device->CreateTexture2D(&texdesc, nullptr, &NormalVStaging);

		texdesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		texdesc.Usage = D3D11_USAGE_DEFAULT;
		texdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		texdesc.CPUAccessFlags = 0;
		hr = Renderer::Device->CreateTexture2D(&texdesc, nullptr, &DepthStencilBuffer);
		hr = Renderer::Device->CreateDepthStencilView(DepthStencilBuffer, nullptr, &DepthStencilBufferDSV);
		texdesc.Usage = D3D11_USAGE_STAGING;
		texdesc.BindFlags = 0;
		texdesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		texdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		hr = Renderer::Device->CreateTexture2D(&texdesc, nullptr, &DepthStencilStaging);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		ID3D11RenderTargetView* rtvs[] = { PositionUBufferRTV, NormalVBufferRTV };
		Renderer::ImmediateContext->OMSetRenderTargets(2, rtvs, DepthStencilBufferDSV);
		D3D11_VIEWPORT viewport = { 0, 0, texdesc.Width, texdesc.Height, 0.0f, 1.0f };
		Renderer::ImmediateContext->RSSetViewports(1, &viewport);

		ID3D11Buffer* vertexBuffer = nullptr;
		UINT stride = sizeof(RenderVertex);
		UINT offset = 0;

		std::vector<BakeLayer> bakeLayers;

		for (int m = 0; m < scene->GetObjectCount(); m++)
		{
			BakeObject* object = scene->GetObject(m);
			DirectX::SimpleMath::Matrix transform = object->BuildTransform();

			for (int s = 0; s < object->GetSectionCount(); s++)
			{
				BakeObject::Section* section = object->GetSection(s);
				if (section->GetMaterial() == material)
				{
					// Rasterize `section`!
					Renderer::ImmediateContext->IASetIndexBuffer(section->GetPreviewMesh()->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
					vertexBuffer = section->GetPreviewMesh()->GetVertexBuffer();
					Renderer::ImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
					int depthMax = 0;
					for (int depthSlice = 0; depthSlice <= depthMax; depthSlice++)
					{
						size_t rayCount = 0;

						// Clear buffers
						Renderer::ImmediateContext->ClearRenderTargetView(PositionUBufferRTV, clearColor);
						Renderer::ImmediateContext->ClearRenderTargetView(NormalVBufferRTV, clearColor);
						Renderer::ImmediateContext->ClearDepthStencilView(DepthStencilBufferDSV, D3D11_CLEAR_STENCIL, 1.0f, 0x00);

						// Draw slice
						Renderer::ImmediateContext->OMSetDepthStencilState(LayerDSS, depthSlice);
						Renderer::ImmediateContext->DrawIndexed(section->GetPreviewMesh()->GetIndexCount(), 0, 0);

						// Copy to staging resources
						Renderer::ImmediateContext->CopyResource(PositionUStaging, PositionUBuffer);
						Renderer::ImmediateContext->CopyResource(NormalVStaging, NormalVBuffer);
						Renderer::ImmediateContext->CopyResource(DepthStencilStaging, DepthStencilBuffer);

						// Copy data out to CPU
						PositionUPixel* positionUData = nullptr;
						D3D11_MAPPED_SUBRESOURCE positionUSubresource = { };
						NormalVPixel* normalVData = nullptr;
						D3D11_MAPPED_SUBRESOURCE normalVSubresource = { };
						DepthStencilPixel* depthStencilData = nullptr;
						D3D11_MAPPED_SUBRESOURCE depthStencilSubresource = { };
						hr = Renderer::ImmediateContext->Map(PositionUStaging, 0, D3D11_MAP_READ, 0, &positionUSubresource);
						hr = Renderer::ImmediateContext->Map(NormalVStaging, 0, D3D11_MAP_READ, 0, &normalVSubresource);
						hr = Renderer::ImmediateContext->Map(DepthStencilStaging, 0, D3D11_MAP_READ, 0, &depthStencilSubresource);
						positionUData = (PositionUPixel*)positionUSubresource.pData;
						normalVData = (NormalVPixel*)normalVSubresource.pData;
						depthStencilData = (DepthStencilPixel*)depthStencilSubresource.pData;

						texels.clear();
						
						for (int y = 0; y < texdesc.Height; y++)
						{
							for (int x = 0; x < texdesc.Width; x++)
							{
								PositionUPixel& positionU = positionUData[y * texdesc.Width + x];
								NormalVPixel& normalV = normalVData[y * texdesc.Width + x];
								DepthStencilPixel& depthStencil = depthStencilData[y * texdesc.Width + x];

								if (depthStencil.Stencil > depthMax + 1)
								{
									depthMax = depthStencil.Stencil - 1;
								}
								if (depthStencil.Stencil == depthSlice + 1)
								{
									// Record this texel
									DirectX::SimpleMath::Vector4 normalVFloat = DirectX::PackedVector::XMLoadHalf4(&normalV.NormalV);
									texels.push_back(TexelRasterization(x, y, positionU.Position, DirectX::SimpleMath::Vector2(positionU.U, normalVFloat.w), DirectX::SimpleMath::Vector3(normalVFloat.x, normalVFloat.y, normalVFloat.z), rayCount));
									rayCount += sampleCount;
								}
							}
						}

						Renderer::ImmediateContext->Unmap(PositionUStaging, 0);
						Renderer::ImmediateContext->Unmap(NormalVStaging, 0);
						Renderer::ImmediateContext->Unmap(DepthStencilStaging, 0);

						if (texels.size() > 0)
						{
							BakeLayer newLayer = BakeLayer(texels.size());
							newLayer.Texels = new TexelRasterization[texels.size()]; // Deleted in Cleanup!
							memcpy(&newLayer.Texels[0], &texels[0], texels.size() * sizeof(TexelRasterization));
							bakeLayers.push_back(newLayer);
							sliceCount++;
						}
					}
				}
			}
		}

		DepthStencilStaging->Release();
		DepthStencilBufferDSV->Release();
		DepthStencilBuffer->Release();
		NormalVStaging->Release();
		NormalVBufferRTV->Release();
		NormalVBuffer->Release();
		PositionUStaging->Release();
		PositionUBufferRTV->Release();
		PositionUBuffer->Release();

		TextureBakeData* newBakeData = new TextureBakeData(material->GetAOTexture(), texdesc.Width, texdesc.Height); // Deleted in Cleanup!
		newBakeData->LayerCount = bakeLayers.size();
		newBakeData->Layers = new BakeLayer[newBakeData->LayerCount]; // Deleted in TextureBakeData::~TextureBakeData
		memcpy(&newBakeData->Layers[0], &bakeLayers[0], bakeLayers.size() * sizeof(BakeLayer));
		textureBakeDatas.push_back(newBakeData);
	}

	// HACK: Get frame to show on graphics analyzer
	//MessageBoxA(0, "Click the capture frame button!", "Dev", 0);
	Renderer::SwapChain->Present(1, 0);

	// Step 2: Generate geometry & acceleration structure (In sync on background thread)
	OutputDebugStringW(L"Processing Scene...\n");
	IntersectionApi* api = IntersectionApi::Create(0);
	std::vector<Shape*> shapes;
	for (int i = 0; i < scene->GetObjectCount(); i++)
	{
		BakeObject* object = scene->GetObject(i);
		for (int s = 0; s < object->GetSectionCount(); s++)
		{
			BakeObject::Section* section = object->GetSection(s);
			RenderMesh* mesh = section->GetPreviewMesh();

			Vector3* positions = new Vector3[mesh->GetVertexCount()];
			for (int p = 0; p < mesh->GetVertexCount(); p++)
			{
				positions[p] = mesh->GetVertices()[p].Position;
			}
			int* primitiveLengths = new int[mesh->GetIndexCount() / 3];
			for (int p = 0; p < mesh->GetIndexCount() / 3; p++)
			{
				primitiveLengths[p] = 3;
			}

			Shape* shape = api->CreateMesh((const float*)positions, mesh->GetVertexCount(), sizeof(Vector3), (const int*)mesh->GetIndices(), 0, primitiveLengths, mesh->GetIndexCount() / 3);
			matrix transform = *(matrix*)&object->BuildTransform();
			matrix invtransform = *(matrix*)&(object->BuildTransform().Invert());
			shape->SetTransform(transform, invtransform);
			api->AttachShape(shape);
			shapes.push_back(shape);

			delete[] primitiveLengths;
			delete[] positions;
		}
	}
	api->Commit();

	OutputDebugStringW(L"Generating samples...\n");
	Vector3* hemisphere = new Vector3[rayChunkSize];
	if (!randomizeRays) {
#ifndef COMPUTE_SHADER
		GenerateHemisphereSamples(hemisphere, rayChunkSize);
#else
		// Upload new hemisphere samples
		D3D11_MAPPED_SUBRESOURCE mappedHemisphere = { };
		Renderer::ImmediateContext->Map(HemisphereBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedHemisphere);
		GenerateHemisphereSamples((Vector3*)mappedHemisphere.pData, rayChunkSize);
		Renderer::ImmediateContext->Unmap(HemisphereBuffer, 0);
#endif
	}
	size_t slice = 0;
	size_t chunkTexelMaxSize = rayChunkSize / sampleCount;
	if (chunkTexelMaxSize > 65535) // D3D11's max threads in one dispatch in one dimension
	{
		chunkTexelMaxSize = 65535;
	}
	for (TextureBakeData* texData : textureBakeDatas)
	{
		for (int i = 0; i < texData->LayerCount; i++)
		{
			slice++;
			static char buffer[100];
			sprintf_s(buffer, "Baking slice %d of %d...\n", (int)slice, (int)sliceCount);
			OutputDebugStringA(buffer);

			size_t startTexel = 0;
			size_t chunkTexelCount = 0;
			while (startTexel < texData->Layers[i].TexelCount)
			{
				chunkTexelCount = texData->Layers[i].TexelCount - startTexel;
				if (chunkTexelCount > chunkTexelMaxSize)
				{
					chunkTexelCount = chunkTexelMaxSize;
				}



				// Step 3: Generate rays
				sprintf_s(buffer, "  Generating Rays for chunk %d of %d...\n", (int)startTexel / (chunkTexelMaxSize), (int)ceil((float)texData->Layers[i].TexelCount / (chunkTexelMaxSize)));
				OutputDebugStringA(buffer);
				if (randomizeRays)
					GenerateHemisphereSamples(hemisphere, rayChunkSize);

#ifndef COMPUTE_SHADER
				ray* rays = (ray*)operator new(chunkTexelCount * sampleCount * sizeof(ray));
				for (int texel = startTexel; texel < startTexel + chunkTexelCount; texel++)
				{
					/*if (randomizeRays)
						GenerateHemisphereSamples(hemisphere, rayChunkSize);*/

					Vector3& hemiY = texData->Layers[i].Texels[texel].Normal;
					Vector3 hemiX = Vector3::UnitX;
					Vector3 hemiZ = Vector3::UnitZ;
					if (0.9999 < hemiY.Dot(Vector3::UnitY) || hemiY.Dot(Vector3::UnitY) < -0.9999)
					{
						hemiY = hemiY;
					}
					else
					{
						hemiX = hemiY.Cross(Vector3::UnitY);
						hemiX.Normalize();
						hemiZ = hemiX.Cross(hemiY);
						hemiZ.Normalize();
					}

					size_t rayIndex = (texel - startTexel) * sampleCount;
					for (int sample = 0; sample < sampleCount; sample++)
					{
						Vector3 pos = texData->Layers[i].Texels[texel].Position + hemiY * 0.001f;
						Vector3& hemi = hemisphere[rayIndex];
						Vector3 dir = Vector3(hemi.x * hemiX.x + hemi.y * hemiY.x + hemi.z * hemiZ.x, hemi.x * hemiX.y + hemi.y * hemiY.y + hemi.z * hemiZ.y, hemi.x * hemiX.z + hemi.y * hemiY.z + hemi.z * hemiZ.z);
					
						rays[rayIndex].o = float4(pos.x, pos.y, pos.z, 1000000.0f);
						rays[rayIndex].d = float3(dir.x, dir.y, dir.z);
						rayIndex++;
					}
				}
#else
				// Upload texels
				D3D11_MAPPED_SUBRESOURCE mappedTexels = { };
				Renderer::ImmediateContext->Map(TexelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedTexels);
				memcpy(mappedTexels.pData, &texData->Layers[i].Texels[startTexel], chunkTexelCount * sizeof(TexelRasterization));
				Renderer::ImmediateContext->Unmap(TexelBuffer, 0);

				Renderer::ImmediateContext->CSSetShader(GenerateRaysComputeShader, nullptr, 0);
				ID3D11ShaderResourceView* cssrvs[] = { TexelBufferSRV, HemisphereBufferSRV };
				Renderer::ImmediateContext->CSSetShaderResources(0, ARRAYSIZE(cssrvs), cssrvs);
				ID3D11UnorderedAccessView* csuavs[] = { RayOutputBufferUAV };
				Renderer::ImmediateContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(csuavs), csuavs, nullptr);
				ID3D11Buffer* cscbs[] = { GenerateRaysConstantBuffer->GetBuffer() };
				Renderer::ImmediateContext->CSSetConstantBuffers(0, ARRAYSIZE(cscbs), cscbs);
				Renderer::ImmediateContext->Dispatch(chunkTexelCount, 1, 1);

				// Populate `rays` from `RayOutputBuffer` (compute shader results)
				Renderer::ImmediateContext->CopyResource(RayOutputStagingBuffer, RayOutputBuffer);
				D3D11_MAPPED_SUBRESOURCE mappedRays = { };
				Renderer::ImmediateContext->Map(RayOutputStagingBuffer, 0, D3D11_MAP_READ, 0, &mappedRays);
				ray* rays = (ray*)mappedRays.pData;
#endif
				Buffer* rayBuffer = api->CreateBuffer(chunkTexelCount * sampleCount * sizeof(ray), rays);

				// Step 4: Launch jobs (In sync on background thread)
				OutputDebugStringW(L"  Casting Rays...\n");
				Buffer* intersectionBuffer = api->CreateBuffer(chunkTexelCount * sampleCount * sizeof(Intersection), nullptr);
				Event* intersectionCompleteEvent = nullptr;
				api->QueryIntersection(rayBuffer, chunkTexelCount * sampleCount, intersectionBuffer, nullptr, &intersectionCompleteEvent);
				intersectionCompleteEvent->Wait();

				// Step 5: Calculate results (In sync on background thread)
				OutputDebugStringW(L"  Processing Results...\n");
				Event* intersectionMapCompleteEvent = nullptr;
				Intersection* intersectionData = nullptr;
				api->MapBuffer(intersectionBuffer, MapType::kMapRead, 0, chunkTexelCount * sampleCount * sizeof(Intersection), (void**)&intersectionData, &intersectionMapCompleteEvent);
				intersectionMapCompleteEvent->Wait();

				// Read ray distances and store the resulting radiances in TextureBakeData.Values
				for (int j = startTexel; j < startTexel + chunkTexelCount; j++)
				{
					TexelRasterization& texel = texData->Layers[i].Texels[j];
					for (int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++)
					{
						ray& ray = rays[(j - startTexel) * sampleCount + sampleIndex];
						Intersection& intersection = intersectionData[(j - startTexel) * sampleCount + sampleIndex];
						float weight = texel.Normal.x * ray.d.x + texel.Normal.y * ray.d.y + texel.Normal.z * ray.d.z;
						float result = 0.0f;
						if (intersection.shapeid != kNullId && intersection.uvwt.w < falloffDistance)
						{
							result = DistanceToRadiance(intersection.uvwt.w, falloffDistance);
							//result = (ray.d.y < -0.9999 || ray.d.y > 0.9999) ? 0.0f : 1.0f;
						}
						else
						{
							result = 1.0f; // DistanceToRadiance(falloffDistance, falloffDistance);
							//result = (ray.d.y < -0.9999 || ray.d.y > 0.9999) ? 0.0f : 1.0f;
						}
						texData->Values[texel.TexelY * texData->Width + texel.TexelX].Add(result, weight);
					}
				}

				Event* intersectionUnmapCompleteEvent = nullptr;
				api->UnmapBuffer(intersectionBuffer, intersectionData, &intersectionUnmapCompleteEvent);
				intersectionUnmapCompleteEvent->Wait();
				api->DeleteBuffer(rayBuffer);
				api->DeleteBuffer(intersectionBuffer);
#ifndef COMPUTE_SHADER
				operator delete(rays);
#else
				Renderer::ImmediateContext->Unmap(RayOutputStagingBuffer, 0);
#endif


				startTexel += chunkTexelCount;
			}

		}
	}
	delete[] hemisphere;

	// HACK: Get frame to show on graphics analyzer
	//MessageBoxA(0, "Click the capture frame button!", "Dev", 0);
	Renderer::SwapChain->Present(1, 0);

	for (Shape* shape : shapes)
	{
		api->DeleteShape(shape);
	}
	IntersectionApi::Delete(api);

	// Step 6: Merge data
	OutputDebugStringW(L"Generating Image...\n");
	for (TextureBakeData* bakeData : textureBakeDatas)
	{
		unsigned char* textureData = bakeData->BakeTexture->GetData();
		for (int y = 0; y < bakeData->Height; y++)
		{
			for (int x = 0; x < bakeData->Width; x++)
			{
				unsigned char value = (bakeData->Values[(size_t)y * bakeData->Width + x].GetAverage()) * 0xFF;
				textureData[(((size_t)y * bakeData->Width + x) * bakeData->BakeTexture->GetChannelCount()) * bakeData->BakeTexture->GetBytesPerChannel()] = value;
				textureData[(((size_t)y * bakeData->Width + x) * bakeData->BakeTexture->GetChannelCount() + 1) * bakeData->BakeTexture->GetBytesPerChannel()] = value;
				textureData[(((size_t)y * bakeData->Width + x) * bakeData->BakeTexture->GetChannelCount() + 2) * bakeData->BakeTexture->GetBytesPerChannel()] = value;
			}
		}
		bakeData->BakeTexture->RefreshPreview();
	}

	// Step 7: Cleanup!
	for (const TextureBakeData* const tex : textureBakeDatas)
	{
		for (int i = 0; i < tex->LayerCount; i++)
		{
			delete[] tex->Layers[i].Texels;
		}
		delete tex;
	}
}