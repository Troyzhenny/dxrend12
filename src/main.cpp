#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
#include <iostream>
#include "d3dx12.h" // directx-headers not available in windows sdk


#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "user32.lib") // for window creation

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// Globals
const UINT WIDTH = 800;
const UINT HEIGHT = 600;
const TCHAR* WndClassName = "Window Blueprint";
const TCHAR* WndTitle = "Dxrend 12 Triangle";

// Forward Decl
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Shader code - will be moved later
const char* vertexShader = R"(
	struct VS_INPUT {
		float3 pos : POSITION;
	};

	struct PS_INPUT 
	{
		float4 pos : SV_POSITION;
		float4 color : COLOR;
	};

	PS_INPUT main(VS_INPUT input)
	{
		PS_INPUT output;
		output.pos = float4(input.pos, 1.0f);

		// Assign colors based on vertex position
		output.color = float4(clamp(input.pos.xyz + 0.5f, 0.0f, 1.0f), 1.0f);

		return output;
	};
)";

const char* pixelShader = R"(
	struct PS_INPUT {
		float4 pos : SV_POSITION;
		float4 color : COLOR;
	};

	float4 main(PS_INPUT input) : SV_TARGET
	{
		return input.color;
	};
)";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	// Window setup
	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = WndClassName;

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, "Call to RegisterClass failed!", "Don't know it mash up!", NULL);
		return 1;
	}

	HWND hwnd = CreateWindow(
		WndClassName,
		WndTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		WIDTH, HEIGHT,
		nullptr, nullptr, hInstance, nullptr
	);

	if (!hwnd)
	{
		MessageBox(NULL, "Call to CreateWindow failed!", "Error", NULL);
		return -1;
	}

	ShowWindow(hwnd, nCmdShow);

	// DX12 Setup
	UINT dxgiFactoryFlags = 0;

	// Create device
	ComPtr<ID3D12Device> device;
	if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
	{
		MessageBox(nullptr, "Failed to create D3D12 device!", "Error", MB_OK);
		return -1;
	}

	// Create command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	ComPtr<ID3D12CommandQueue> commandQueue;
	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));

	// Create swapchain
	ComPtr<IDXGIFactory4> factory;
	CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2; // front and back
	swapChainDesc.Width = WIDTH;
	swapChainDesc.Height = HEIGHT;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	factory->CreateSwapChainForHwnd(
		commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain);
	ComPtr<IDXGISwapChain3> swapChain3;
	swapChain.As(&swapChain3);

	// Create descriptor heaps for render target views
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Create render target views
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandleStart(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	ComPtr<ID3D12Resource> renderTargets[2];

	for (UINT i = 0; i < 2; i++)
	{
		swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
		device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandleStart);
		rtvHandleStart.Offset(1, rtvDescriptorSize);
	}

	// Create command allocator
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));

	// Create root signature
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

	ComPtr<ID3D12RootSignature> rootSignature;
	device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	// Compile Shaders
	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;
	ComPtr<ID3DBlob> errorBlob;

	if (FAILED(D3DCompile(vertexShader, strlen(vertexShader), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexShaderBlob, &errorBlob)))
	{
		MessageBoxA(nullptr, (char*)errorBlob->GetBufferPointer(), "Vertex Shader Error", MB_OK);
		return 1;
	}

	if (FAILED(D3DCompile(pixelShader, strlen(pixelShader), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &pixelShaderBlob, &errorBlob)))
	{
		MessageBoxA(nullptr, (char*)errorBlob->GetBufferPointer(), "Pixel Shader Error", MB_OK);
		return 1;
	}

	// Define vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDesc, _countof(inputElementDesc) };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	ComPtr<ID3D12PipelineState> pipelineState;
	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));

	// Create command list
	ComPtr<ID3D12GraphicsCommandList> commandList;
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&commandList));
	commandList->Close(); // close before first reset in the loop

	// Create vertex buffer
	struct Vertex {
		XMFLOAT3 position;
	};

	Vertex vertices[] = {
		{XMFLOAT3(0.0f, 0.5f, 0.0f)},   // top center
		{XMFLOAT3(0.5f, -0.5f, 0.0f)},  // bottom right
		{XMFLOAT3(-0.5f, -0.5f, 0.0f)}  // bottom left
	};

	const UINT vertexBufferSize = sizeof(vertices);

	ComPtr<ID3D12Resource> vertexBuffer;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

	device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer)
	);

	// Copy vertex data
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	memcpy(pVertexDataBegin, vertices, vertexBufferSize);
	vertexBuffer->Unmap(0, nullptr);

	// Init vertex buffer view
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = vertexBufferSize;

	// Create synchronization objects
	ComPtr<ID3D12Fence> fence;
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	UINT64 fenceValue = 1;
	HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// Main loop
	UINT frameIndex = swapChain3->GetCurrentBackBufferIndex();
	bool running = true;
	MSG msg = {};

	while (running) {
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				running = false;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Reset command allocator and command list
		commandAllocator->Reset();
		commandList->Reset(commandAllocator.Get(), pipelineState.Get());

		// Set necessary state
		commandList->SetGraphicsRootSignature(rootSignature.Get());

		CD3DX12_VIEWPORT viewport(0.0f, 0.0f, (float)WIDTH, (float)HEIGHT);
		CD3DX12_RECT scissorRect(0, 0, WIDTH, HEIGHT);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);

		// Indicate that the back buffer will be used as a render target
		CD3DX12_RESOURCE_BARRIER barrierToRT = CD3DX12_RESOURCE_BARRIER::Transition(
			renderTargets[frameIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &barrierToRT);

		// Record commands
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
			rtvHeap->GetCPUDescriptorHandleForHeapStart(),
			frameIndex,
			rtvDescriptorSize);

		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandList->DrawInstanced(3, 1, 0, 0);

		// Indicate that the back buffer will now be used to present
		CD3DX12_RESOURCE_BARRIER barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
			renderTargets[frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &barrierToPresent);

		// Execute command list
		commandList->Close();
		ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
		commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Present the frame
		swapChain->Present(1, 0);

		// Wait for frame to complete
		const UINT64 currentFenceValue = fenceValue;
		commandQueue->Signal(fence.Get(), currentFenceValue);
		fenceValue++;

		if (fence->GetCompletedValue() < currentFenceValue) {
			fence->SetEventOnCompletion(currentFenceValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		frameIndex = swapChain3->GetCurrentBackBufferIndex();
	}

	// Cleanup
	CloseHandle(fenceEvent);

	return 0;
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}