#define UNICODE
#include "Core/Instance.h"
#include "Core/Device.h"
#include "iostream"
#include <vector>
#include <DirectXMath.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <chrono>
#pragma comment(lib, "d3d12.lib")
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 611; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
const std::uint32_t STAGING_BUFFER_SIZE = 2'097'152; //2MB staging buffer
struct Vertex
{
	struct { float x, y, z; } Position;
	struct { float x, y, z; } Color;
};
struct Constants
{
	DirectX::XMFLOAT4X4 model;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
	DirectX::XMFLOAT4X4 _pad;

	DirectX::XMFLOAT4X4 _model;
	DirectX::XMFLOAT4X4 _view;
	DirectX::XMFLOAT4X4 _projection;
	DirectX::XMFLOAT4X4 __pad;

	DirectX::XMFLOAT4X4 __model;
	DirectX::XMFLOAT4X4 __view;
	DirectX::XMFLOAT4X4 __projection;
	DirectX::XMFLOAT4X4 ___pad;

	DirectX::XMFLOAT4X4 ___model;
	DirectX::XMFLOAT4X4 ___view;
	DirectX::XMFLOAT4X4 ___projection;
	DirectX::XMFLOAT4X4 ____pad;
};
LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CLOSE)
	{
		DestroyWindow(hWnd);
	}
	if (uMsg == WM_DESTROY)
	{
		PostQuitMessage(0);
	}
	if (uMsg == WM_SIZE)
	{
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
HWND CreateWin32Window()
{
	WNDCLASSEX wndclass = {};
	wndclass.cbSize = sizeof(wndclass);
	wndclass.hInstance = GetModuleHandleA(NULL);
	wndclass.lpszClassName = L"Window";
	wndclass.lpfnWndProc = WndProc;
	RegisterClassEx(&wndclass);
	RECT winRect;
	winRect.left = 0;
	winRect.right = 1280;
	winRect.top = 0;
	winRect.bottom = 720;
	AdjustWindowRectEx(&winRect, WS_OVERLAPPEDWINDOW, NULL, NULL);
	HWND hwnd = CreateWindowEx(WS_EX_ACCEPTFILES, L"Window", L"Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, winRect.right - winRect.left, winRect.bottom - winRect.top, NULL, NULL, GetModuleHandleA(NULL), NULL);
	ShowWindow(hwnd, SW_SHOW);
	return hwnd;
}
void InitializeRHI(RHI::Instance** inst, RHI::PhysicalDevice** phys_device, RHI::Device** device, RHI::CommandQueue** queue,RHI::SwapChain** swapChain, RHI::Texture** backBufferImages, RHI::Texture** depthBufferImage, RHI::DescriptorHeap** rtvHeap,RHI::DescriptorHeap** dsvHeap, HWND hwnd)
{
	RHI::CommandQueueDesc cmdDesc;
	cmdDesc.CommandListType = RHI::CommandListType::Direct;
	cmdDesc.Priority = 0.f;

	RHI::Instance::Create(inst);
	(*inst)->GetPhysicalDevice(0, phys_device);
	RHI::PhysicalDeviceDesc desc;
	(*phys_device)->GetDesc(&desc);
	std::wcout << desc.Description << std::endl;
	RHI::Device::Create(*phys_device, &cmdDesc, 1, queue, device);

	RHI::Surface surface;
	surface.InitWin32(hwnd, (*inst)->ID);
	RHI::SwapChainDesc sDesc(RHI::Default);
	sDesc.OutputSurface = surface;
	sDesc.Width = 1280;
	sDesc.Height = 720;
	sDesc.SwapChainFormat = RHI::Format::B8G8R8A8_UNORM;

	(*inst)->CreateSwapChain(&sDesc, *phys_device, *device, *queue, swapChain);
	(*device)->GetSwapChainImage(*swapChain, 0, &backBufferImages[0]);
	(*device)->GetSwapChainImage(*swapChain, 1, &backBufferImages[1]);

	RHI::PoolSize pSize;
	pSize.numDescriptors = 2;
	pSize.type = RHI::DescriptorType::RTV;
	RHI::DescriptorHeapDesc rtvHeapHesc;
	rtvHeapHesc.maxDescriptorSets = 1;
	rtvHeapHesc.numPoolSizes = 1;
	rtvHeapHesc.poolSizes = &pSize;
	(*device)->CreateDescriptorHeap(&rtvHeapHesc, rtvHeap);

	RHI::ClearValue depthVal = {};
	depthVal.format = RHI::Format::D32_FLOAT;
	depthVal.depthStencilValue.depth = 1.f;
	RHI::TextureDesc depthTexture;
	depthTexture.depthOrArraySize = 1;
	depthTexture.format = RHI::Format::D32_FLOAT;
	depthTexture.height = 720;
	depthTexture.mipLevels = 1;
	depthTexture.mode = RHI::TextureTilingMode::Optimal;
	depthTexture.sampleCount = 1;
	depthTexture.type = RHI::TextureType::Texture2D;
	depthTexture.usage = RHI::TextureUsage::DepthStencilAttachment;
	depthTexture.width = 1280;
	depthTexture.optimizedClearValue = &depthVal;
	RHI::HeapProperties props;
	props.type = RHI::HeapType::Default;
	(*device)->CreateTexture(&depthTexture, depthBufferImage, nullptr,&props, 0, RHI::ResourceType::Commited);
	pSize.type = RHI::DescriptorType::DSV;
	pSize.numDescriptors = 1;
	(*device)->CreateDescriptorHeap(&rtvHeapHesc, dsvHeap);
}
int main()
{
	HWND hwnd = CreateWin32Window();
	
	MSG msg = {};

	RHI::Instance* inst = {};
	RHI::Device* device;
	RHI::PhysicalDevice* phys_device;
	RHI::CommandQueue* commandQueue;
	RHI::SwapChain* swapChain = {};
	RHI::Texture* backBufferImages[2] = {};
	RHI::Texture* depthStencilView;
	RHI::DescriptorHeap* rtvDescriptorHeap;
	RHI::DescriptorHeap* dsvDescriptorHeap;
	
	RHI::Buffer* VertexBuffer;
	RHI::Buffer* ConstantBuffer;
	RHI::Buffer* IndexBuffer;
	RHI::Buffer* StagingBuffer;
	RHI::Texture* texture;
	RHI::Texture* Depthtexture = nullptr;
	RHI::PipelineStateObject* pso;
	RHI::Heap* StagingHeap;
	RHI::Heap* heap;
	RHI::Heap* cbheap;
	RHI::RootSignature* rSig;
	RHI::DescriptorSetLayout* dsLayout[2];
	RHI::DescriptorHeap* dHeap;
	RHI::DescriptorHeap* SamplerDHeap;
	RHI::DescriptorSet* dSet[2];
	RHI::Fence* fence;

	InitializeRHI(&inst, &phys_device, &device, &commandQueue, &swapChain, backBufferImages, &Depthtexture,&rtvDescriptorHeap, &dsvDescriptorHeap,hwnd);
	struct FrameResource
	{
		std::uint64_t fence_val = 0;
		RHI::CommandAllocator* commandAllocator;
		int Release() { return commandAllocator->Release(); };
	};
	RHI::GraphicsCommandList* commandList;
	FrameResource fResources[3];
	for (auto& s : fResources)
	{
		device->CreateCommandAllocator(RHI::CommandListType::Direct, &s.commandAllocator);
	}
	device->CreateFence(&fence, 0);
	device->CreateCommandList(RHI::CommandListType::Direct, fResources[1].commandAllocator, &commandList);
	std::vector<std::uint16_t> indices{
		//Top
		2, 6, 7,
		2, 3, 7,

		//Bottom
		0, 4, 5,
		0, 1, 5,

		//Left
		0, 2, 6,
		0, 4, 6,

		//Right
		1, 3, 7,
		1, 5, 7,

		//Front
		0, 2, 3,
		0, 1, 3,

		//Back
		4, 6, 7,
		4, 5, 7
	};
	const std::vector<Vertex> vertices = {
	{{-1, -1,  0.5,}, { 0.0f, 0.0f,1.f,}},
	{{ 1, -1,  0.5,}, { 1.0f, 0.0f,1.f,}},
	{{-1,  1,  0.5,}, { 1.0f, 1.0f,1.f,}},
	{{ 1,  1,  0.5,}, { 1.0f, 1.0f,0.f,}},
	{{-1, -1, -0.5,}, { 0.0f, 1.0f,0.f,}},
	{{ 1, -1, -0.5,}, { 0.0f, 1.0f,1.f,}},
	{{-1,  1, -0.5,}, { 0.3f, 0.3f, 0.4f}},
	{{ 1,  1, -0.5 }, { 0.0f, 0.0f,1.f,}},
	};

	Constants c;
	DirectX::XMStoreFloat4x4(&c.model, DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(0.f))));
	DirectX::XMStoreFloat4x4(&c._model, DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(1.f, 5.f, -4.f, 1.f), DirectX::XMVectorZero(), DirectX::XMVectorSet(0, 1, 0, 0))));
	DirectX::XMStoreFloat4x4(&c.__model, DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45.f), 16.f / 9.f, 0.1f, 100.f)));

	DirectX::XMVECTOR v;
	v = DirectX::XMVectorSet(0, 1.f, 0, 1);
	v = DirectX::XMVector3Transform( v, DirectX::XMMatrixTranslation(0.3f, 0, 0));
	v = DirectX::XMVector3Transform( v, DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(0.f, 1.f, -3.f, 1.f), DirectX::XMVectorZero(), DirectX::XMVectorSet(0, 1, 0, 0)));
	v = DirectX::XMVector3Transform(v, DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45.f), 16.f/9.f, 0.1f, 100.f));
	DirectX::XMFLOAT4 f;
	DirectX::XMStoreFloat4(&f, v);
	std::cout << f.x << " " << f.y << " " << f.z << std::endl;

	RHI::BufferDesc stagingBufferDesc;
	stagingBufferDesc.size = 1024 * 1024 * 16;
	stagingBufferDesc.usage = RHI::BufferUsage::CopySrc;

	RHI::BufferDesc bufferDesc;
	bufferDesc.size = sizeof(Vertex) * vertices.size();
	bufferDesc.usage = RHI::BufferUsage::VertexBuffer | RHI::BufferUsage::CopyDst;

	RHI::BufferDesc ConstantbufferDesc;
	ConstantbufferDesc.size = sizeof(Constants);
	ConstantbufferDesc.usage = RHI::BufferUsage::ConstantBuffer | RHI::BufferUsage::CopyDst;

	RHI::BufferDesc IndexBufferDesc;
	IndexBufferDesc.size = sizeof(std::uint16_t) * indices.size();
	IndexBufferDesc.usage = RHI::BufferUsage::IndexBuffer | RHI::BufferUsage::CopyDst;

	int width, height, numChannels;
	void* image_data = stbi_load("texture.jpg", &width, &height, &numChannels, STBI_rgb_alpha);
	std::uint32_t imageByteSize = width * height * 4;

	RHI::TextureDesc textureDesc;
	textureDesc.depthOrArraySize = 1;
	textureDesc.format = RHI::Format::R8G8B8A8_UNORM;
	textureDesc.height = height;
	textureDesc.width = width;
	textureDesc.mipLevels = 1;
	textureDesc.mode = RHI::TextureTilingMode::Optimal;
	textureDesc.sampleCount = 1;
	textureDesc.type = RHI::TextureType::Texture2D;
	textureDesc.usage = RHI::TextureUsage::CopyDst | RHI::TextureUsage::SampledImage;
	

	RHI::MemoryReqirements bufferMemRequirements;
	RHI::MemoryReqirements ConstantbufferMemRequirements;
	RHI::MemoryReqirements IndexbufferMemRequirements;
	RHI::MemoryReqirements StagingbufferMemRequirements;
	RHI::MemoryReqirements textureMemRequirements;
	device->GetBufferMemoryRequirements(&bufferDesc, &bufferMemRequirements);
	device->GetBufferMemoryRequirements(&ConstantbufferDesc, &ConstantbufferMemRequirements);
	device->GetBufferMemoryRequirements(&IndexBufferDesc, &IndexbufferMemRequirements);
	device->GetBufferMemoryRequirements(&stagingBufferDesc, &StagingbufferMemRequirements);
	device->GetTextureMemoryRequirements(&textureDesc, &textureMemRequirements);

	RHI::HeapDesc StagingHeapDesc;
	bool fallback = true;
	StagingHeapDesc.props.type = RHI::HeapType::Custom;
	StagingHeapDesc.props.FallbackType = RHI::HeapType::Upload;
	StagingHeapDesc.props.memoryLevel = RHI::MemoryLevel::DedicatedRAM;
	StagingHeapDesc.props.pageProperty = RHI::CPUPageProperty::Any;
	StagingHeapDesc.size = StagingbufferMemRequirements.size;
	device->CreateHeap(&StagingHeapDesc, &StagingHeap, &fallback);

	RHI::HeapDesc CBHeapDesc;
	CBHeapDesc.props.type = RHI::HeapType::Custom;
	CBHeapDesc.props.memoryLevel = RHI::MemoryLevel::DedicatedRAM;
	CBHeapDesc.props.pageProperty = RHI::CPUPageProperty::Any;
	CBHeapDesc.size = ConstantbufferMemRequirements.size;
	device->CreateHeap(&CBHeapDesc, &cbheap, nullptr);

	device->CreateBuffer(&stagingBufferDesc, &StagingBuffer, StagingHeap, nullptr, 0, RHI::ResourceType::Placed);

	RHI::HeapDesc heapDesc;
	heapDesc.props.type = RHI::HeapType::Default;
	heapDesc.size = bufferMemRequirements.size + textureMemRequirements.size + IndexbufferMemRequirements.size + 1024;
	device->CreateHeap(&heapDesc, &heap, &fallback);
		
	uint32_t offset = 0;
	device->CreateBuffer(&bufferDesc, &VertexBuffer, heap, nullptr, offset, RHI::ResourceType::Placed);
	offset = (((bufferDesc.size + offset) / ConstantbufferMemRequirements.alignment) + 1)* ConstantbufferMemRequirements.alignment;
	device->CreateBuffer(&IndexBufferDesc, &IndexBuffer, heap,nullptr,  offset, RHI::ResourceType::Placed);
	device->CreateBuffer(&ConstantbufferDesc, &ConstantBuffer, cbheap, nullptr, 0, RHI::ResourceType::Placed);
	offset = (((IndexBufferDesc.size + offset) / textureMemRequirements.alignment) + 1) * textureMemRequirements.alignment;
	device->CreateTexture(&textureDesc, &texture, heap, nullptr, offset, RHI::ResourceType::Placed);
	
	void* data = nullptr;

	ConstantBuffer->Map(&data);
	memcpy(data, &c, sizeof(Constants));
	ConstantBuffer->UnMap();
	
	RHI::TextureMemoryBarrier imagebarr;
	imagebarr.AccessFlagsBefore = RHI::ResourceAcessFlags::NONE;
	imagebarr.AccessFlagsAfter = RHI::ResourceAcessFlags::TRANSFER_WRITE;
	imagebarr.oldLayout = RHI::ResourceLayout::UNDEFINED;
	imagebarr.newLayout = RHI::ResourceLayout::TRANSFER_DST_OPTIMAL;
	imagebarr.texture = texture;
	imagebarr.subresourceRange = {
		.imageAspect = RHI::Aspect::COLOR_BIT,
		.IndexOrFirstMipLevel = 0,
		.NumMipLevels = 1,
		.FirstArraySlice = 0,
		.NumArraySlices = 1,
	};

	//Copy from staging buffer
	commandList->Begin(fResources[1].commandAllocator);
	commandList->PipelineBarrier(RHI::PipelineStage::TOP_OF_PIPE_BIT, RHI::PipelineStage::TRANSFER_BIT, 0, nullptr, 1, &imagebarr);

	StagingBuffer->Map(&data);

	std::uint32_t vbOffset = sizeof(std::uint16_t) * indices.size();
	std::uint32_t imgOffset = vbOffset + sizeof(Vertex) * vertices.size();
	memcpy(data, indices.data(), sizeof(std::uint16_t)* indices.size());
	memcpy((char*)data + vbOffset, vertices.data(), sizeof(Vertex) * vertices.size());
	memcpy((char*)data + imgOffset, image_data, imageByteSize);

	StagingBuffer->UnMap();
	commandList->CopyBufferRegion(0, 0, sizeof(std::uint16_t)* indices.size(), StagingBuffer, IndexBuffer);
	commandList->CopyBufferRegion(vbOffset, 0, sizeof(Vertex)* vertices.size(), StagingBuffer, VertexBuffer);
	commandList->CopyBufferToImage(imgOffset, width, height, imagebarr.subresourceRange, { 0,0,0 }, { 512,512,1 }, StagingBuffer, texture);

	imagebarr.oldLayout = RHI::ResourceLayout::TRANSFER_DST_OPTIMAL;
	imagebarr.newLayout = RHI::ResourceLayout::SHADER_READ_ONLY_OPTIMAL;
	imagebarr.AccessFlagsBefore = RHI::ResourceAcessFlags::TRANSFER_WRITE;
	imagebarr.AccessFlagsAfter = RHI::ResourceAcessFlags::SHADER_READ;
	commandList->PipelineBarrier(RHI::PipelineStage::TRANSFER_BIT, RHI::PipelineStage::FRAGMENT_SHADER_BIT, 0, nullptr, 1, &imagebarr);
	commandList->End();

	commandQueue->ExecuteCommandLists(&commandList->ID, 1);
	commandQueue->SignalFence(fence, 1);
	fence->Wait(1);
	commandQueue->SignalFence(fence, 0);
	stbi_image_free(image_data);
	RHI::PipelineStateObjectDesc PSOdesc = {};
	PSOdesc.VS = "shaders/basic_triangle_vs";
	PSOdesc.PS = "shaders/basic_triangle_fs";
	PSOdesc.topology = RHI::PrimitiveTopology::TriangleList;

	RHI::InputElementDesc ied[2];
	ied[0].location = 0;
	ied[0].inputSlot = 0;
	ied[0].format = RHI::Format::R32G32B32_FLOAT;
	ied[0].alignedByteOffset = offsetof(Vertex, Position);

	ied[1].location = 1;
	ied[1].inputSlot = 0;
	ied[1].format = RHI::Format::R32G32B32_FLOAT;
	ied[1].alignedByteOffset = offsetof(Vertex, Color);

	RHI::InputBindingDesc ibd;
	ibd.inputRate = RHI::InputRate::Vertex;
	ibd.stride = sizeof(Vertex);

	RHI::DescriptorRange range[5];
	range[0].numDescriptors = 1;
	range[0].BaseShaderRegister = 0;
	range[0].type  = range[1].type = range[2].type = range[3].type = RHI::DescriptorType::CBV;

	range[1].numDescriptors = 1;
	range[1].BaseShaderRegister = 1;

	range[2].numDescriptors = 1;
	range[2].BaseShaderRegister = 0;

	range[3].numDescriptors = 1;
	range[3].BaseShaderRegister = 1;

	range[4].BaseShaderRegister = 2;
	range[4].numDescriptors = 1;
	range[4].type = RHI::DescriptorType::SRV;

	RHI::RootParameterDesc rpDesc[2];
	rpDesc[0].type = rpDesc[1].type = RHI::RootParameterType::DescriptorTable;
	rpDesc[0].descriptorTable.numDescriptorRanges = 2;
	rpDesc[0].descriptorTable.ranges = range;
	rpDesc[1].descriptorTable.ranges = &range[2];
	rpDesc[1].descriptorTable.numDescriptorRanges = 3;


	RHI::RootSignatureDesc rsDesc;
	rsDesc.numRootParameters = 2;
	rsDesc.rootParameters = rpDesc;
	device->CreateRootSignature(&rsDesc, &rSig, dsLayout);

	PSOdesc.inputBindings = &ibd;
	PSOdesc.inputElements = ied;
	PSOdesc.numInputBindings = 1;
	PSOdesc.numInputElements = 2;
	PSOdesc.rootSig = rSig;
	PSOdesc.DepthEnabled = true;
	PSOdesc.DSVFormat = RHI::Format::D32_FLOAT;

	device->CreatePipelineStateObject(&PSOdesc, &pso);

	RHI::PoolSize pSize[2];
	pSize[0].numDescriptors = 10;
	pSize[0].type = RHI::DescriptorType::CBV;
	pSize[1].type = RHI::DescriptorType::SRV;
	pSize[1].numDescriptors = 2;

	RHI::DescriptorHeapDesc dhDesc;
	dhDesc.maxDescriptorSets = 10;
	dhDesc.numPoolSizes = 2;
	dhDesc.poolSizes = pSize;

	device->CreateDescriptorHeap(&dhDesc, &dHeap);
	dhDesc.maxDescriptorSets = 5;
	dhDesc.numPoolSizes = 1;
	pSize[0].type = RHI::DescriptorType::Sampler;
	device->CreateDescriptorHeap(&dhDesc, &SamplerDHeap);
	device->CreateDescriptorSets(dHeap, 1, dsLayout[0], &dSet[0]);
	device->CreateDescriptorSets(dHeap, 1, dsLayout[1], &dSet[1]);

	RHI::DescriptorBufferInfo bufferInfo[4];
	bufferInfo[0].buffer = bufferInfo[1].buffer = bufferInfo[2].buffer= bufferInfo[3].buffer =  ConstantBuffer;
	bufferInfo[0].offset = 0;
	bufferInfo[1].offset = 256;
	bufferInfo[2].offset = 256 *2;
	bufferInfo[3].offset = 256 *3;
	bufferInfo[0].range = bufferInfo[1].range = bufferInfo[2].range = bufferInfo[3].range = 256;

	RHI::DescriptorTextureInfo Tinfo;
	Tinfo.texture = texture;

	RHI::DescriptorSetUpdateDesc dhuDesc[5];
	dhuDesc[0].arrayIndex = 0;
	dhuDesc[0].binding = 0;
	dhuDesc[0].bufferInfos = &bufferInfo[0];
	dhuDesc[0].numDescriptors = 1;
	dhuDesc[0].type = RHI::DescriptorType::CBV;

	dhuDesc[1].arrayIndex = 0;
	dhuDesc[1].binding = 1;
	dhuDesc[1].bufferInfos = &bufferInfo[1];
	dhuDesc[1].numDescriptors = 1;
	dhuDesc[1].type = RHI::DescriptorType::CBV;

	dhuDesc[2].arrayIndex = 0;
	dhuDesc[2].binding = 0;
	dhuDesc[2].bufferInfos = &bufferInfo[2];
	dhuDesc[2].numDescriptors = 1;
	dhuDesc[2].type = RHI::DescriptorType::CBV;

	dhuDesc[3].arrayIndex = 0;
	dhuDesc[3].binding = 1;
	dhuDesc[3].bufferInfos = &bufferInfo[3];
	dhuDesc[3].numDescriptors = 1;
	dhuDesc[3].type = RHI::DescriptorType::CBV;

	dhuDesc[4].arrayIndex = 0;
	dhuDesc[4].binding = 2;
	dhuDesc[4].textureInfos = &Tinfo;
	dhuDesc[4].numDescriptors = 1;
	dhuDesc[4].type = RHI::DescriptorType::SRV;

	device->UpdateDescriptorSets(2, dhuDesc, dSet[0]);
	device->UpdateDescriptorSets(3, &dhuDesc[2], dSet[1]);

	RHI::CPU_HANDLE CurrentRtvHandle = rtvDescriptorHeap->GetCpuHandle();
	RHI::CPU_HANDLE handles[2];
	for (int i = 0; i < 2; i++)
	{
		RHI::CPU_HANDLE handle;
		handle.val = CurrentRtvHandle.val + (i * device->GetDescriptorHeapIncrementSize(RHI::DescriptorType::RTV));
		device->CreateRenderTargetView(backBufferImages[i], nullptr, handle);

		handles[i] = handle;
	}
	device->CreateDepthStencilView(Depthtexture, nullptr, dsvDescriptorHeap->GetCpuHandle());

	uint64_t fenceVal = 0;
	uint32_t currentRtvIndex = 0;
	bool running = true;
	
	const std::uint32_t framesInFlight = 3;
	std::uint32_t currentFrame = 0;
	RHI::RenderingAttachmentDesc CAdesc;
	CAdesc.clearColor = { 0.2f, 1.f, 0.4f, 1.f };
	CAdesc.loadOp = RHI::LoadOp::Clear;
	CAdesc.storeOp = RHI::StoreOp::Store;
	RHI::RenderingAttachmentDesc DAdesc;
	DAdesc.clearColor = { 1.f, 1.f, 1.f, 1.f };
	DAdesc.loadOp = RHI::LoadOp::Clear;
	DAdesc.storeOp = RHI::StoreOp::DontCare;
	DAdesc.ImageView = dsvDescriptorHeap->GetCpuHandle();
	RHI::Area2D render_area;
	render_area.offset = { 0,0 };
	render_area.size = { 1280, 720 };
	RHI::RenderingBeginDesc RBdesc = {};
	RBdesc.numColorAttachments = 1;
	RBdesc.renderingArea = render_area;
	RBdesc.pColorAttachments = &CAdesc;
	RBdesc.pDepthStencilAttachment = &DAdesc;
	RHI::Viewport vp;
	vp.x = 0;  vp.y = 0;
	vp.width = 1280;
	vp.height = 720;
	vp.minDepth = 0.f;
	vp.maxDepth = 1.f;
	std::chrono::time_point clock = std::chrono::high_resolution_clock::now();
	while (running)
	{
		if (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			if (msg.message == WM_QUIT)
				running = false;
		}
		if (!running)
			break;
		std::chrono::time_point clock2 = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(clock2 - clock).count();
		DirectX::XMStoreFloat4x4(&c.model, DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(time * 10))));
		ConstantBuffer->Map(&data);
		memcpy(data, &c, sizeof(c));
		ConstantBuffer->UnMap();
		currentFrame = (currentFrame + 1) % framesInFlight;

		fence->Wait(fResources[currentFrame].fence_val);
		fResources[currentFrame].commandAllocator->Reset();
		commandList->Begin(fResources[currentFrame].commandAllocator);
		DirectX::XMColorEqual(v, DirectX::XMVectorZero());
		CAdesc.ImageView = (handles[currentRtvIndex]);
		
		RHI::TextureMemoryBarrier barr;
		barr.AccessFlagsBefore = RHI::ResourceAcessFlags::NONE;
		barr.AccessFlagsAfter = RHI::ResourceAcessFlags::COLOR_ATTACHMENT_WRITE;
		barr.oldLayout = RHI::ResourceLayout::UNDEFINED;
		barr.newLayout = RHI::ResourceLayout::COLOR_ATTACHMENT_OPTIMAL;
		barr.subresourceRange = {
			.imageAspect = RHI::Aspect::COLOR_BIT,
			.IndexOrFirstMipLevel = 0,
			.NumMipLevels = 1,
			.FirstArraySlice = 0,
			.NumArraySlices = 1,
		};
		barr.texture  = backBufferImages[currentRtvIndex];
		
		commandList->PipelineBarrier(RHI::PipelineStage::TOP_OF_PIPE_BIT, RHI::PipelineStage::COLOR_ATTACHMENT_OUTPUT_BIT, 0, nullptr, 1, & barr);
		commandList->BeginRendering(&RBdesc);

		commandList->SetPipelineState(pso);
		commandList->SetRootSignature(rSig);
		commandList->SetDescriptorHeap(dHeap);
		commandList->BindDescriptorSet(rSig, dSet[0], 0);
		commandList->BindDescriptorSet(rSig, dSet[1], 1);
		commandList->SetViewports(1, &vp);
		commandList->SetScissorRects(1, &render_area);
		Internal_ID vertexBuffers [] = {VertexBuffer->ID};
		commandList->BindVertexBuffers(0, 1, vertexBuffers);
		commandList->BindIndexBuffer(IndexBuffer, 0);
		commandList->DrawIndexed(indices.size(), 1, 0, 0, 0);

		commandList->EndRendering();
		// Transition the swapchain image back to a presentable layout
		barr.oldLayout = (RHI::ResourceLayout::COLOR_ATTACHMENT_OPTIMAL);
		barr.newLayout = (RHI::ResourceLayout::PRESENT);
		barr.AccessFlagsBefore = (RHI::ResourceAcessFlags::COLOR_ATTACHMENT_WRITE);
		barr.AccessFlagsAfter = (RHI::ResourceAcessFlags::NONE);
		commandList->PipelineBarrier( RHI::PipelineStage::COLOR_ATTACHMENT_OUTPUT_BIT, RHI::PipelineStage::BOTTOM_OF_PIPE_BIT, 0,0,1, & barr);

		// End command buffer recording
		commandList->End();

		// Submit the command buffer to the queue
		Internal_ID ids[1] = { commandList->ID };
		commandQueue->ExecuteCommandLists(ids, 1);

		fResources[currentFrame].fence_val = ++fenceVal;
		commandQueue->SignalFence(fence, fenceVal);

		swapChain->Present(currentRtvIndex);

		currentRtvIndex = (currentRtvIndex + 1) % 2;
	}
	VertexBuffer->Hold();
	VertexBuffer->Release();
	IndexBuffer->Release();
	ConstantBuffer->Release();
	StagingBuffer->Release();
	dsLayout[0]->Release();
	dsLayout[1]->Release();
	dSet[0]->Release();
	dSet[1]->Release();
	commandList->Release();
	swapChain->Release();
	fResources[0].Release();
	fResources[1].Release();
	commandQueue->Release();
	rSig->Release();
	pso->Release();
}