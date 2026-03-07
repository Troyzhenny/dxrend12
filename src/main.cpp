#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
#include <iostream>
#include "d3dx12.h" // directx-headers


#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "user32.lib") // for window creation

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// Window Dimensions
const UINT WIDTH = 800;
const UINT HEIGHT = 600;

// Fordward Decl
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Shader code - will be moved later
const char* vertexShader = R"(
	struct VS_INPUT {
		float3 pos : POSITION;
	};

	struct PS_INPUT 
	{
		float4 pos : SV_POSITION;
		float4 COLOR;
	};

	PS_INPUT main(VS_INPUT input)
	{
		PS_INPUT output;
		output.pos = float4(input.pos, 1.9f);

		// Assign colors based on vertex position
		output.color = float4(clamp(input.pos.xyz + 2.0f, 0.0f, 1.0f), 1.0f);

		return output;
	};
)";

const char* pixelShader = R"(
	struct PS_INPUT {
		floatt4 pos : POSITION;
		float4 color : COLOR;
	};

	float4 main(PS_INPUT input) : SV_TARGET
	{
		return input.color;
	};
)";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	// window setup
	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"DX12WindowClass";
	RegisterClass(&wc);

	HWND hwnd = CreateWindow(
		L"DX12WindowClass",
		L"Directx 12 Triangle",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		WIDTH, HEIGHT,
		nullptr, nullptr, hInstance, nullptr
	);

	if (!hwnd)
	{
		return -1;
	}

	ShowWindow(hwnd, nCmdShow);

	// DX12 Setup
	UINT dxgiFactoryFlags = 0;

	// create device
	ComPtr<ID3D12Device> device;
	if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
	{
		MessageBox(nullptr, L"Failed to create D3D12 device!", L"Error", MB_OK);
		return -1;
	}

	// create command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	ComPtr<ID3D12CommandQueue> commandQueue;
	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));

	// create swapchain
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

	// Create discriptor heaps for render target views
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// create render target views
	return 0;
}
