// Copyright (C), UNIGINE. All rights reserved.

#pragma once

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <UnigineResourceExternalMemory.h>
#include <UnigineResourceFence.h>
#include <UnigineLog.h>

using Microsoft::WRL::ComPtr;

#ifdef __GNUC__
	#define __UNIGINE_FUNCTION__ __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
	#define __UNIGINE_FUNCTION__ __FUNCTION__
#endif

#ifndef DX_CHECK
	#define DX_CHECK(x) \
	{ \
		HRESULT hr_ = (x); \
		if (FAILED(hr_)) \
			Unigine::Log::fatal("%s: %s Failed with HRESULT 0x%08X\n", __UNIGINE_FUNCTION__, #x, (unsigned int)hr_); \
	}
#endif

// Creates a D3D11 device (ID3D11Device5 + ID3D11DeviceContext4).
// Uses the default adapter. Feature level 11_1 minimum.
// Note: on multi-GPU systems the default adapter may differ from the engine's GPU.
void createD3D11Device(ComPtr<ID3D11Device5> &device, ComPtr<ID3D11DeviceContext4> &context);

// Opens a shared Win32 handle from ResourceExternalMemory as an ID3D11Buffer.
void openSharedBuffer(ID3D11Device5 *device, Unigine::ResourceExternalMemoryPtr rem, ComPtr<ID3D11Buffer> &buffer);

// Opens a shared Win32 handle from ResourceExternalMemory as an ID3D11Texture2D.
void openSharedTexture2D(ID3D11Device5 *device, Unigine::ResourceExternalMemoryPtr rem, ComPtr<ID3D11Texture2D> &texture);

// Opens a shared fence handle as ID3D11Fence.
void openSharedFence(ID3D11Device5 *device, Unigine::ResourceFencePtr fence, ComPtr<ID3D11Fence> &dx_fence);

// Compiles an HLSL compute shader at runtime. hlsl_path is a virtual path (relative to
// the sample's data_path); it is resolved to a real filesystem path via the engine's
// FileSystem so compilation works regardless of the process working directory.
ComPtr<ID3D11ComputeShader> compileComputeShader(ID3D11Device5 *device, const char *hlsl_path, const char *entry_point = "CSMain");
