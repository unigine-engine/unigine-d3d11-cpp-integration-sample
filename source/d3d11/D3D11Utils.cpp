// Copyright (C), UNIGINE. All rights reserved.

#include "D3D11Utils.h"

#include <UnigineRender.h>
#include <UnigineSystemInfo.h>
#include <UnigineFileSystem.h>
#include <UnigineString.h>

using namespace Unigine;

void createD3D11Device(ComPtr<ID3D11Device5> &device, ComPtr<ID3D11DeviceContext4> &context)
{
	UINT flags = 0;
	#ifndef NDEBUG
		flags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif

	D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1};
	D3D_FEATURE_LEVEL feature_level_out{};

	// Find the DXGI adapter matching the engine's GPU via LUID.
	// OpenSharedResource1 requires the D3D11 device to be on the same adapter
	// as the D3D12 device that created the shared resource.
	unsigned long long engine_luid = SystemInfo::getGPULuid();

	ComPtr<IDXGIFactory1> factory;
	DX_CHECK(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(factory.GetAddressOf())));

	ComPtr<IDXGIAdapter> adapter;
	for (UINT i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++)
	{
		DXGI_ADAPTER_DESC desc{};
		adapter->GetDesc(&desc);
		unsigned long long adapter_luid = (static_cast<unsigned long long>(desc.AdapterLuid.HighPart) << 32) | desc.AdapterLuid.LowPart;
		if (adapter_luid == engine_luid)
			break;
		adapter.Reset();
	}

	if (!adapter)
		Log::fatal("createD3D11Device: could not find DXGI adapter matching engine GPU (LUID 0x%016llX)\n", engine_luid);

	ComPtr<ID3D11Device> base_device;
	ComPtr<ID3D11DeviceContext> base_context;

	DX_CHECK(D3D11CreateDevice(
		adapter.Get(),
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		flags,
		feature_levels,
		_countof(feature_levels),
		D3D11_SDK_VERSION,
		&base_device,
		&feature_level_out,
		&base_context));

	if (flags & D3D11_CREATE_DEVICE_DEBUG)
	{
		ComPtr<ID3D11Debug> debug;
		base_device.As(&debug);

		debug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);
		ComPtr<ID3D11InfoQueue> info_queue;
		if (SUCCEEDED(base_device.As(&info_queue)))
		{
			info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);

			// Optionally filter or clear messages here
		}
	}

	DX_CHECK(base_device.As(&device));
	DX_CHECK(base_context.As(&context));
}

void openSharedBuffer(ID3D11Device5 *device, Unigine::ResourceExternalMemoryPtr rem, ComPtr<ID3D11Buffer> &buffer)
{
	HANDLE handle = rem->getWin32Handle();
	DX_CHECK(device->OpenSharedResource1(handle, __uuidof(ID3D11Buffer), reinterpret_cast<void **>(buffer.ReleaseAndGetAddressOf())));
	rem->closeHandle();
}

void openSharedTexture2D(ID3D11Device5 *device, Unigine::ResourceExternalMemoryPtr rem, ComPtr<ID3D11Texture2D> &texture)
{
	HANDLE handle = rem->getWin32Handle();
	DX_CHECK(device->OpenSharedResource1(handle, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(texture.ReleaseAndGetAddressOf())));
	rem->closeHandle();
}

void openSharedFence(ID3D11Device5 *device, Unigine::ResourceFencePtr fence, ComPtr<ID3D11Fence> &dx_fence)
{
	HANDLE handle = fence->getWin32Handle();
	DX_CHECK(device->OpenSharedFence(handle, __uuidof(ID3D11Fence), reinterpret_cast<void **>(dx_fence.ReleaseAndGetAddressOf())));
	fence->closeHandle();
}

ComPtr<ID3D11ComputeShader> compileComputeShader(ID3D11Device5 *device, const char *hlsl_path, const char *entry_point)
{
	// Resolve the virtual path (relative to the sample's data_path) to a real filesystem
	// path, then convert to a wide string for D3DCompileFromFile.
	String abs_path = FileSystem::getAbsolutePath(hlsl_path);
	wchar_t wide_path[1024]{};
	if (MultiByteToWideChar(CP_UTF8, 0, abs_path.get(), -1, wide_path, _countof(wide_path)) == 0)
		Log::fatal("compileComputeShader: cannot resolve shader path \"%s\"\n", hlsl_path);

	UINT compile_flags = 0;
	#ifdef _DEBUG
		compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#else
		compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
	#endif

	ComPtr<ID3DBlob> shader_blob;
	ComPtr<ID3DBlob> error_blob;

	HRESULT hr = D3DCompileFromFile(
		wide_path,
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entry_point,
		"cs_5_0",
		compile_flags,
		0,
		&shader_blob,
		&error_blob);

	if (FAILED(hr))
	{
		if (error_blob)
			Log::fatal("Shader compilation failed: %s\n", (const char *)error_blob->GetBufferPointer());
		else
			Log::fatal("Shader compilation failed with HRESULT 0x%08X\n", (unsigned int)hr);
	}

	ComPtr<ID3D11ComputeShader> shader;
	DX_CHECK(device->CreateComputeShader(
		shader_blob->GetBufferPointer(),
		shader_blob->GetBufferSize(),
		nullptr,
		&shader));

	return shader;
}
