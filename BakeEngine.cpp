#include "BakeEngine.h"

#include "Renderer.h"

#include "Render/RenderTexture.h"

#include "Scene/Scene.h"
#include "Render/ConstantBuffer.h"
#include "Scene/BakeTexture.h"

#include "RadeonRays/radeon_rays.h"

const int sampleCount = 100;
const float falloffDistance = 1.25f;
const bool randomizeRays = false;
const bool useAnyHit = true; // UseAnyHit means no max AO distance - any hit, no matter how far away, counts as occluded.
const int rayChunkSize = 0xFFFFFF;
const int bleedDistance = 5;

ID3D11DepthStencilState* LayerDSS = nullptr;
ID3D11RasterizerState* NoCullRS = nullptr;

struct GenerateRaysOptions
{
	int SampleCount;
	int TextureWidth;
	int UseAnyHit;
	float FalloffDistance;
};

struct PostprocessVertex
{
	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector2 TexCoord;
};

const PostprocessVertex PostprocessVertices[] = {
	{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
};
const unsigned int PostprocessIndices[] = {
	0, 1, 2,
	0, 2, 3,
};
ID3D11Buffer* PostprocessVertexBuffer = nullptr;
ID3D11Buffer* PostprocessIndexBuffer = nullptr;

const D3D11_INPUT_ELEMENT_DESC RasterizeInputElements[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0}
};
const D3D11_INPUT_ELEMENT_DESC PostprocessInputElements[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
const std::wstring RasterizeVertexShaderFilename = L"data/RasterizeVertexShader.cso";
const std::wstring RasterizePixelShaderFilename = L"data/RasterizePixelShader.cso";
const std::wstring GenerateRaysComputeShaderFilename = L"data/GenerateRaysComputeShader.cso";
const std::wstring ProcessIntersectionsComputeShaderFilename = L"data/ProcessIntersectionsComputeShader.cso";
const std::wstring PostprocessVertexShaderFilename = L"data/PostprocessVertexShader.cso";
const std::wstring PostprocessPixelShaderFilename = L"data/PostprocessPixelShader.cso";
RenderShader* RasterizeShader = nullptr;
RenderShader* PostprocessShader = nullptr;
ID3D11ComputeShader* GenerateRaysComputeShader = nullptr;
ID3D11ComputeShader* ProcessIntersectionsComputeShader = nullptr;
ConstantBuffer<GenerateRaysOptions>* GenerateRaysConstantBuffer = nullptr;

ID3D11Buffer* TexelBuffer = nullptr;
ID3D11ShaderResourceView* TexelBufferSRV = nullptr;
ID3D11Buffer* HemisphereBuffer = nullptr;
ID3D11ShaderResourceView* HemisphereBufferSRV = nullptr;
ID3D11Buffer* RayOutputBuffer = nullptr;
ID3D11ShaderResourceView* RayOutputBufferSRV = nullptr;
ID3D11UnorderedAccessView* RayOutputBufferUAV = nullptr;
ID3D11Buffer* RayOutputStagingBuffer = nullptr;
ID3D11Buffer* IntersectionBuffer = nullptr;
ID3D11ShaderResourceView* IntersectionBufferSRV = nullptr;

struct ObjectConstants
{
	DirectX::SimpleMath::Matrix Model;
};
ConstantBuffer<ObjectConstants>* ObjectConstantBuffer = nullptr;

struct PostprocessConstants
{
	int ResolutionX;
	int ResolutionY;
	int BleedDistance;
	int _Padding;
};
ConstantBuffer<PostprocessConstants>* PostprocessConstantBuffer = nullptr;

struct TexelRasterization
{
	int TexelX;
	int TexelY;
	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector2 TexCoord;
	DirectX::SimpleMath::Vector3 Normal;

	TexelRasterization() : TexelRasterization(-1, -1, DirectX::SimpleMath::Vector3(), DirectX::SimpleMath::Vector2(), DirectX::SimpleMath::Vector3())
	{

	}

	TexelRasterization(int texelX, int texelY, DirectX::SimpleMath::Vector3 position, DirectX::SimpleMath::Vector2 texCoord, DirectX::SimpleMath::Vector3 normal)
		: TexelX(texelX), TexelY(texelY), Position(position), TexCoord(texCoord), Normal(normal)
	{

	}
};

void BakeEngine::Init()
{
	HRESULT hr;
	D3D11_BUFFER_DESC vbdesc = CD3D11_BUFFER_DESC(sizeof(PostprocessVertex) * ARRAYSIZE(PostprocessVertices), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE);
	D3D11_SUBRESOURCE_DATA vbdata = { };
	vbdata.pSysMem = (void*)PostprocessVertices;
	hr = Renderer::Device->CreateBuffer(&vbdesc, &vbdata, &PostprocessVertexBuffer);
	D3D11_BUFFER_DESC ibdesc = CD3D11_BUFFER_DESC(sizeof(unsigned int) * ARRAYSIZE(PostprocessIndices), D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_IMMUTABLE);
	D3D11_SUBRESOURCE_DATA ibdata = { };
	ibdata.pSysMem = (void*)PostprocessIndices;
	hr = Renderer::Device->CreateBuffer(&ibdesc, &ibdata, &PostprocessIndexBuffer);

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
	hr = Renderer::Device->CreateDepthStencilState(&dsdesc, &LayerDSS);

	D3D11_RASTERIZER_DESC rsdesc = { };
	rsdesc.CullMode = D3D11_CULL_NONE;
	rsdesc.FillMode = D3D11_FILL_SOLID;
	hr = Renderer::Device->CreateRasterizerState(&rsdesc, &NoCullRS);

	RasterizeShader = RenderShader::Create(RasterizeInputElements, ARRAYSIZE(RasterizeInputElements), RasterizeVertexShaderFilename, RasterizePixelShaderFilename);
	PostprocessShader = RenderShader::Create(PostprocessInputElements, ARRAYSIZE(PostprocessInputElements), PostprocessVertexShaderFilename, PostprocessPixelShaderFilename);

	ObjectConstantBuffer = ConstantBuffer<ObjectConstants>::Create(Renderer::Device, ObjectConstants{ DirectX::SimpleMath::Matrix() }, true);
	PostprocessConstantBuffer = ConstantBuffer<PostprocessConstants>::Create(Renderer::Device, { 0, 0, 0, 0 }, true);

	ID3DBlob* generateRaysBytecode = Renderer::ReadBlobFromFile(GenerateRaysComputeShaderFilename);
	hr = Renderer::Device->CreateComputeShader(generateRaysBytecode->GetBufferPointer(), generateRaysBytecode->GetBufferSize(), nullptr, &GenerateRaysComputeShader);
	generateRaysBytecode->Release();

	ID3DBlob* processIntersectionsBytecode = Renderer::ReadBlobFromFile(ProcessIntersectionsComputeShaderFilename);
	hr = Renderer::Device->CreateComputeShader(processIntersectionsBytecode->GetBufferPointer(), processIntersectionsBytecode->GetBufferSize(), nullptr, &ProcessIntersectionsComputeShader);
	processIntersectionsBytecode->Release();

	GenerateRaysConstantBuffer = ConstantBuffer<GenerateRaysOptions>::Create(Renderer::Device, { 0, 0, 0, 0.0f }, true);

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
	bufdesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufdesc.ByteWidth = rayChunkSize * sizeof(RadeonRays::ray);
	bufdesc.CPUAccessFlags = 0;
	bufdesc.StructureByteStride = sizeof(RadeonRays::ray);
	bufdesc.Usage = D3D11_USAGE_DEFAULT;
	hr = Renderer::Device->CreateBuffer(&bufdesc, nullptr, &RayOutputBuffer);
	srvdesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(RayOutputBuffer, DXGI_FORMAT_UNKNOWN, 0, rayChunkSize);
	hr = Renderer::Device->CreateShaderResourceView(RayOutputBuffer, &srvdesc, &RayOutputBufferSRV);
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
	// Intersection buffer
	bufdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bufdesc.ByteWidth = rayChunkSize * sizeof(RadeonRays::Intersection);
	bufdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufdesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufdesc.StructureByteStride = sizeof(RadeonRays::Intersection);
	bufdesc.Usage = D3D11_USAGE_DYNAMIC;
	hr = Renderer::Device->CreateBuffer(&bufdesc, nullptr, &IntersectionBuffer);
	hr = Renderer::Device->CreateShaderResourceView(IntersectionBuffer, &srvdesc, &IntersectionBufferSRV);
}

void BakeEngine::Dispose()
{
	PostprocessIndexBuffer->Release();
	PostprocessIndexBuffer = nullptr;

	PostprocessVertexBuffer->Release();
	PostprocessVertexBuffer = nullptr;

	IntersectionBufferSRV->Release();
	IntersectionBufferSRV = nullptr;

	IntersectionBuffer->Release();
	IntersectionBuffer = nullptr;

	RayOutputStagingBuffer->Release();
	RayOutputStagingBuffer = nullptr;

	RayOutputBufferUAV->Release();
	RayOutputBufferUAV = nullptr;

	RayOutputBufferSRV->Release();
	RayOutputBufferSRV = nullptr;

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

	ProcessIntersectionsComputeShader->Release();
	ProcessIntersectionsComputeShader = nullptr;

	GenerateRaysComputeShader->Release();
	GenerateRaysComputeShader = nullptr;

	delete PostprocessConstantBuffer;
	PostprocessConstantBuffer = nullptr;

	delete ObjectConstantBuffer;
	ObjectConstantBuffer = nullptr;

	delete PostprocessShader;
	PostprocessShader = nullptr;

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

struct ResultValue
{
	int QuantizedTotal = 0;
	int QuantizedTotalWeight = 0;

	int GetAverage()
	{
		if (this->QuantizedTotalWeight == 0)
			return 0;
		return (QuantizedTotal * 255) / QuantizedTotalWeight;
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
		//buffer[i] = DirectX::SimpleMath::Vector3(x, y, z); // This line was taking 34% of total bake time!!!!!!!!
		buffer[i].x = x;
		buffer[i].y = y;
		buffer[i].z = z;
	}
}

void BakeEngine::Bake(Scene* scene)
{
	using namespace DirectX::SimpleMath;
	using namespace RadeonRays;

	static char buffer[100];

	// HACK: Get frame to show on graphics analyzer
	//MessageBoxA(0, "Click the capture frame button!", "Dev", 0);
	Renderer::SwapChain->Present(1, 0);

	// Step 0: Upload the compute shader settings!
	// Generate hemisphere
	Vector3* hemisphere = new Vector3[rayChunkSize];
	if (!randomizeRays) {
		// Upload new hemisphere samples
		OutputDebugStringW(L"Generating random samples...\n");
		D3D11_MAPPED_SUBRESOURCE mappedHemisphere = { };
		Renderer::ImmediateContext->Map(HemisphereBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedHemisphere);
		GenerateHemisphereSamples((Vector3*)mappedHemisphere.pData, rayChunkSize);
		Renderer::ImmediateContext->Unmap(HemisphereBuffer, 0);
	}

	// Compute chunk size
	size_t slice = 0;
	size_t chunkTexelMaxSize = rayChunkSize / sampleCount;
	if (chunkTexelMaxSize > 65535) // D3D11's max threads in one dispatch in one dimension
	{
		chunkTexelMaxSize = 65535;
	}

	// Step 1: Generate geometry & acceleration structure (In sync on background thread)
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
			matrix transform = (*(matrix*)&object->BuildTransform()).transpose();
			matrix invtransform = (*(matrix*)&(object->BuildTransform().Invert())).transpose();
			api->AttachShape(shape);
			shape->SetTransform(transform, invtransform);
			shapes.push_back(shape);

			delete[] primitiveLengths;
			delete[] positions;
		}
	}
	api->Commit();

	HRESULT hr = S_OK;
	std::vector<TexelRasterization> texels;
	for (int i = 0; i < scene->GetMaterialCount(); i++)
	{
		sprintf_s(buffer, "Material %d of %d...\n", i, scene->GetMaterialCount());
		OutputDebugStringA(buffer);

		MaterialObject* material = scene->GetMaterial(i);

		ID3D11Texture2D* PositionUBuffer = nullptr; // R32G32B32A32 (X, Y, Z, U)
		ID3D11Texture2D* PositionUStaging = nullptr;
		ID3D11RenderTargetView* PositionUBufferRTV = nullptr;
		ID3D11Texture2D* NormalVBuffer = nullptr; // R16G16B16A16 (NX, NY, NZ, V)
		ID3D11ShaderResourceView* NormalVBufferSRV = nullptr;
		ID3D11RenderTargetView* NormalVBufferRTV = nullptr;
		ID3D11Texture2D* NormalVStaging = nullptr;
		ID3D11Texture2D* DepthStencilBuffer = nullptr; // D24S8 (Depth Unused, Stencil)
		ID3D11Texture2D* DepthStencilStaging = nullptr;
		ID3D11DepthStencilView* DepthStencilBufferDSV = nullptr;
		ID3D11Buffer* ResultBuffer = nullptr;
		ID3D11UnorderedAccessView* ResultBufferUAV = nullptr;
		ID3D11ShaderResourceView* ResultBufferSRV = nullptr;
		ID3D11Texture2D* FinalTexture = nullptr;
		ID3D11RenderTargetView* FinalTextureRTV = nullptr;
		ID3D11Texture2D* FinalTextureStaging = nullptr;

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
		texdesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texdesc.CPUAccessFlags = 0;
		hr = Renderer::Device->CreateTexture2D(&texdesc, nullptr, &NormalVBuffer);
		hr = Renderer::Device->CreateShaderResourceView(NormalVBuffer, nullptr, &NormalVBufferSRV);
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

		D3D11_BUFFER_DESC bufdesc = CD3D11_BUFFER_DESC((size_t)texdesc.Width * texdesc.Height * sizeof(ResultValue), D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);
		bufdesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufdesc.StructureByteStride = sizeof(ResultValue);
		hr = Renderer::Device->CreateBuffer(&bufdesc, nullptr, &ResultBuffer);
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavdesc = { };
		uavdesc.Format = DXGI_FORMAT_UNKNOWN;
		uavdesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavdesc.Buffer.FirstElement = 0;
		uavdesc.Buffer.NumElements = (size_t)texdesc.Width * texdesc.Height;
		hr = Renderer::Device->CreateUnorderedAccessView(ResultBuffer, &uavdesc, &ResultBufferUAV);
		hr = Renderer::Device->CreateShaderResourceView(ResultBuffer, nullptr, &ResultBufferSRV);

		texdesc = CD3D11_TEXTURE2D_DESC(material->GetAOTexture()->GetGPUPreview()->GetFormat(), texdesc.Width, texdesc.Height, 1, 1, D3D11_BIND_RENDER_TARGET);
		hr = Renderer::Device->CreateTexture2D(&texdesc, nullptr, &FinalTexture);
		hr = Renderer::Device->CreateRenderTargetView(FinalTexture, nullptr, &FinalTextureRTV);
		texdesc = CD3D11_TEXTURE2D_DESC(texdesc.Format, texdesc.Width, texdesc.Height, 1, 1, 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ);
		hr = Renderer::Device->CreateTexture2D(&texdesc, nullptr, &FinalTextureStaging);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		ID3D11RenderTargetView* rtvs[] = { PositionUBufferRTV, NormalVBufferRTV };
		Renderer::ImmediateContext->OMSetRenderTargets(2, rtvs, DepthStencilBufferDSV);
		D3D11_VIEWPORT viewport = { 0, 0, (int)texdesc.Width, (int)texdesc.Height, 0.0f, 1.0f };
		Renderer::ImmediateContext->RSSetViewports(1, &viewport);

		ID3D11Buffer* vertexBuffer = nullptr;
		UINT stride = sizeof(RenderVertex);
		UINT offset = 0;

		// Send options to the compute shader constants
		GenerateRaysConstantBuffer->Update(Renderer::ImmediateContext, { sampleCount, (int)texdesc.Width, useAnyHit ? 1 : 0, falloffDistance });

		// Step 2: Rasterize into layers (Must be in sync on main thread)
		Renderer::ImmediateContext->IASetInputLayout(RasterizeShader->GetInputLayout());
		Renderer::ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		Renderer::ImmediateContext->VSSetShader(RasterizeShader->GetVertexShader(), nullptr, 0);
		Renderer::ImmediateContext->PSSetShader(RasterizeShader->GetPixelShader(), nullptr, 0);
		Renderer::ImmediateContext->RSSetState(NoCullRS);

		OutputDebugStringW(L"  Baking Slices...\n");
		for (int m = 0; m < scene->GetObjectCount(); m++)
		{
			BakeObject* object = scene->GetObject(m);
			DirectX::SimpleMath::Matrix transform = object->BuildTransform();

			ID3D11Buffer* constantBuffer = nullptr;
			Renderer::ImmediateContext->VSSetConstantBuffers(0, 1, &constantBuffer);
			ObjectConstantBuffer->Update(Renderer::ImmediateContext, ObjectConstants { transform });
			constantBuffer = ObjectConstantBuffer->GetBuffer();
			Renderer::ImmediateContext->VSSetConstantBuffers(0, 1, &constantBuffer);

			for (int s = 0; s < object->GetSectionCount(); s++)
			{
				BakeObject::Section* section = object->GetSection(s);
				if (section->GetMaterial() == material)
				{
					sprintf_s(buffer, "    %s.%s...\n", object->GetNameGUI(), section->GetNameGUI());
					OutputDebugStringA(buffer);
					// Rasterize `section`!
					Renderer::ImmediateContext->IASetIndexBuffer(section->GetPreviewMesh()->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
					vertexBuffer = section->GetPreviewMesh()->GetVertexBuffer();
					Renderer::ImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
					int depthMax = 0;
					for (int depthSlice = 0; depthSlice <= depthMax; depthSlice++)
					{
						sprintf_s(buffer, "      Slice %d of %d...\n", depthSlice, depthMax);
						OutputDebugStringA(buffer);

						OutputDebugStringW(L"        Rasterizing...\n");
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
						
						OutputDebugStringW(L"        Extracting texels...\n");
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
									texels.push_back(TexelRasterization(x, y, positionU.Position, DirectX::SimpleMath::Vector2(positionU.U, normalVFloat.w), DirectX::SimpleMath::Vector3(normalVFloat.x, normalVFloat.y, normalVFloat.z)));
								}
							}
						}

						Renderer::ImmediateContext->Unmap(PositionUStaging, 0);
						Renderer::ImmediateContext->Unmap(NormalVStaging, 0);
						Renderer::ImmediateContext->Unmap(DepthStencilStaging, 0);

						if (texels.size() > 0)
						{
							// Bake texels!
							slice++;
							size_t startTexel = 0;
							size_t chunkTexelCount = 0;
							while (startTexel < texels.size())
							{
								chunkTexelCount = texels.size() - startTexel;
								if (chunkTexelCount > chunkTexelMaxSize)
								{
									chunkTexelCount = chunkTexelMaxSize;
								}

								// Step 3: Generate rays for chunk
								sprintf_s(buffer, "        Chunk %d of %d...\n", (int)startTexel / (chunkTexelMaxSize), (int)ceil((float)texels.size() / (chunkTexelMaxSize)));
								OutputDebugStringA(buffer);
								OutputDebugStringW(L"          Generating Rays...\n");
								if (randomizeRays)
									GenerateHemisphereSamples(hemisphere, rayChunkSize);

								// Upload texels
								D3D11_MAPPED_SUBRESOURCE mappedTexels = { };
								Renderer::ImmediateContext->Map(TexelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedTexels);
								memcpy(mappedTexels.pData, &texels[startTexel], chunkTexelCount * sizeof(TexelRasterization));
								Renderer::ImmediateContext->Unmap(TexelBuffer, 0);

								Renderer::ImmediateContext->CSSetShader(GenerateRaysComputeShader, nullptr, 0);
								ID3D11ShaderResourceView* grcssrvs[] = { TexelBufferSRV, HemisphereBufferSRV };
								Renderer::ImmediateContext->CSSetShaderResources(0, ARRAYSIZE(grcssrvs), grcssrvs);
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
								Buffer* rayBuffer = api->CreateBuffer(chunkTexelCount * sampleCount * sizeof(ray), rays);

								// Step 4: Launch jobs (In sync on background thread): (rayBuffer) -> (intersectionBuffer)
								OutputDebugStringW(L"          Casting Rays...\n");
								Buffer* intersectionBuffer = api->CreateBuffer(chunkTexelCount * sampleCount * sizeof(Intersection), nullptr);
								Event* intersectionCompleteEvent = nullptr;
								api->QueryIntersection(rayBuffer, chunkTexelCount * sampleCount, intersectionBuffer, nullptr, &intersectionCompleteEvent);
								intersectionCompleteEvent->Wait();

								// Step 5: Calculate results (In sync on background thread)
								OutputDebugStringW(L"          Processing Results...\n");
								Event* intersectionMapCompleteEvent = nullptr;
								Intersection* intersectionData = nullptr;
								api->MapBuffer(intersectionBuffer, MapType::kMapRead, 0, chunkTexelCount * sampleCount * sizeof(Intersection), (void**)&intersectionData, &intersectionMapCompleteEvent);
								intersectionMapCompleteEvent->Wait();

								// Read ray distances and store the resulting radiances in TextureBakeData.Values
								//   Upload the intersections
								D3D11_MAPPED_SUBRESOURCE mappedIntersections = { };
								Renderer::ImmediateContext->Map(IntersectionBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedIntersections);
								memcpy(mappedIntersections.pData, (void*)intersectionData, chunkTexelCount * sampleCount * sizeof(Intersection));
								Renderer::ImmediateContext->Unmap(IntersectionBuffer, 0);
								//   Run compute shader: ProcessIntersectionsComputeShader(TexelBufferSRV, IntersectionBufferSRV, RayOutputBufferSRV) -> (ResultBufferUAV)
								Renderer::ImmediateContext->CSSetShader(ProcessIntersectionsComputeShader, nullptr, 0);
								Renderer::ImmediateContext->CSSetUnorderedAccessViews(0, 1, &ResultBufferUAV, nullptr);
								ID3D11ShaderResourceView* picssrvs[] = { TexelBufferSRV, IntersectionBufferSRV, RayOutputBufferSRV };
								Renderer::ImmediateContext->CSSetShaderResources(0, ARRAYSIZE(picssrvs), picssrvs);
								// (Keep the same constant buffer as last time)
								Renderer::ImmediateContext->Dispatch(chunkTexelCount, 1, 1);
								ID3D11ShaderResourceView* noSRV = nullptr;
								Renderer::ImmediateContext->CSSetShaderResources(2, 1, &noSRV);

								Event* intersectionUnmapCompleteEvent = nullptr;
								api->UnmapBuffer(intersectionBuffer, intersectionData, &intersectionUnmapCompleteEvent);
								intersectionUnmapCompleteEvent->Wait();
								api->DeleteBuffer(rayBuffer);
								api->DeleteBuffer(intersectionBuffer);
								Renderer::ImmediateContext->Unmap(RayOutputStagingBuffer, 0);

								startTexel += chunkTexelCount;
							}
						}
					}
				}
			}
		}
		
		// HACK: Get frame to show on graphics analyzer
		//MessageBoxA(0, "Click the capture frame button!", "Dev", 0);
		Renderer::SwapChain->Present(1, 0);
		// Step 6: Merge data
		// TODO: Replace this step with version on GPU: PostprocessShader(ResultBuffer, NormalVBuffer) -> (FinalTexture)
		OutputDebugStringW(L"  Generating Image...\n");
		PostprocessConstantBuffer->Update(Renderer::ImmediateContext, { (int)texdesc.Width, (int)texdesc.Height, bleedDistance, 0 });
		Renderer::ImmediateContext->IASetInputLayout(PostprocessShader->GetInputLayout());
		UINT postprocessStride = sizeof(PostprocessVertex);
		UINT postprocessOffset = 0;
		Renderer::ImmediateContext->IASetVertexBuffers(0, 1, &PostprocessVertexBuffer, &postprocessStride, &postprocessOffset);
		Renderer::ImmediateContext->IASetIndexBuffer(PostprocessIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		Renderer::ImmediateContext->VSSetShader(PostprocessShader->GetVertexShader(), nullptr, 0);
		ID3D11Buffer* postprocessConstantBuffer = PostprocessConstantBuffer->GetBuffer();
		Renderer::ImmediateContext->VSSetConstantBuffers(0, 1, &postprocessConstantBuffer);
		Renderer::ImmediateContext->PSSetShader(PostprocessShader->GetPixelShader(), nullptr, 0);
		Renderer::ImmediateContext->PSSetConstantBuffers(0, 1, &postprocessConstantBuffer);
		ID3D11UnorderedAccessView* emptyUAVs[] = { nullptr };
		Renderer::ImmediateContext->CSSetUnorderedAccessViews(0, 1, emptyUAVs, nullptr); // Unbind ResultBuffer from CS output before using it as PS input
		Renderer::ImmediateContext->OMSetRenderTargets(1, &FinalTextureRTV, nullptr); // Unbind NormalVBuffer from OM output before using it as PS input
		ID3D11ShaderResourceView* postprocessSRVs[] = { ResultBufferSRV, NormalVBufferSRV };
		Renderer::ImmediateContext->PSSetShaderResources(0, ARRAYSIZE(postprocessSRVs), postprocessSRVs);
		Renderer::ImmediateContext->DrawIndexed(ARRAYSIZE(PostprocessIndices), 0, 0);
		Renderer::ImmediateContext->CopyResource(FinalTextureStaging, FinalTexture);
		D3D11_MAPPED_SUBRESOURCE mappedFinalTexture = { };
		hr = Renderer::ImmediateContext->Map(FinalTextureStaging, 0, D3D11_MAP_READ, 0, &mappedFinalTexture);
		memcpy(material->GetAOTexture()->GetData(), mappedFinalTexture.pData, texdesc.Width * texdesc.Height * material->GetAOTexture()->GetChannelCount() * material->GetAOTexture()->GetBytesPerChannel());
		Renderer::ImmediateContext->Unmap(FinalTextureStaging, 0);


		// HACK: Get frame to show on graphics analyzer
		//MessageBoxA(0, "Click the capture frame button!", "Dev", 0);
		Renderer::SwapChain->Present(1, 0);


		FinalTextureStaging->Release();
		FinalTextureRTV->Release();
		FinalTexture->Release();
		DepthStencilStaging->Release();
		DepthStencilBufferDSV->Release();
		DepthStencilBuffer->Release();
		NormalVStaging->Release();
		NormalVBufferRTV->Release();
		NormalVBufferSRV->Release();
		NormalVBuffer->Release();
		PositionUStaging->Release();
		PositionUBufferRTV->Release();
		PositionUBuffer->Release();

		ResultBufferSRV->Release();
		ResultBufferUAV->Release();
		ResultBuffer->Release();
		material->GetAOTexture()->RefreshPreview();

	}

	// HACK: Get frame to show on graphics analyzer
	//MessageBoxA(0, "Click the capture frame button!", "Dev", 0);
	Renderer::SwapChain->Present(1, 0);

	// Step 7: Cleanup!
	delete[] hemisphere;
	for (Shape* shape : shapes)
	{
		api->DeleteShape(shape);
	}
	IntersectionApi::Delete(api);

	OutputDebugStringW(L"Bake Complete.\n");
}