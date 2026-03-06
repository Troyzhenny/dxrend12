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
	UINT DXGIFactoryFlags = 0;

	return 0;
}
