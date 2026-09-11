// Copyright (C), UNIGINE. All rights reserved.

#include "TextureSharedD3D11.h"

#include <UnigineRender.h>

#include "D3D11Utils.h"

using namespace Unigine;

bool TextureSharedD3D11::exportToD3D11(const TexturePtr &src)
{
	clear();

	if ((src->getFormatFlags() & Texture::FORMAT_USAGE_SHARED) == 0)
		return false;

	texture = src;

	// nullptr is returned if no SHARED flag
	resource_external_memory = texture->getResourceExternalMemory();
	if (resource_external_memory == nullptr)
		return false;

	createD3D11Device(device, context);

	openSharedTexture2D(device.Get(), resource_external_memory, dx_texture);

	DXGI_FORMAT format = convert_texture_format();

	// Create UAV for compute shader write access
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
		uav_desc.Format = format;
		uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uav_desc.Texture2D.MipSlice = 0;

		DX_CHECK(device->CreateUnorderedAccessView(dx_texture.Get(), &uav_desc, &uav));
	}

	// Create SRV for read access (used by TextureTransfer)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
		srv_desc.Format = format;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Texture2D.MostDetailedMip = 0;

		DX_CHECK(device->CreateShaderResourceView(dx_texture.Get(), &srv_desc, &srv));
	}

	fence = ResourceFence::create();
	openSharedFence(device.Get(), fence, dx_fence);

	Render::getEventEndFrameExecuteCommandLists().connect(event_connection, [this]()
	{
		if (fence.isValid() == false || fence->isEnabled() == false)
			return;

		if (event_work.empty() == false)
		{
			wait();
			event_work.run();
		}
		signal();
	});

	initialized = true;
	return true;
}

void TextureSharedD3D11::clear()
{
	if (initialized == false)
		return;

	event_connection.disconnect();

	// NOTE:
	// This is not optimal for Texture re-creation between frames
	// as it introduces high-cost GPU <-> CPU Sync point before we can destroy old resources.
	//
	// What you want to do instead is to have double-buffered Textures
	// and to have a separate thread which will carry destruction of the current Texture
	// while the other one is used to render something.
	//
	// Also you can use a single fence for all of your shared resources so that you will have to wait
	// single time instead of waiting for multiple fences, this also may improve performance
	// as far fewer sync Wait/Signal pairs will be issued and waited on.
	context->Flush();
	signal();
	fence->waitGPU();

	srv.Reset();
	uav.Reset();
	dx_texture.Reset();
	dx_fence.Reset();

	texture = nullptr;
	resource_external_memory = nullptr;
	fence = nullptr;

	context.Reset();
	device.Reset();

	initialized = false;
}

void TextureSharedD3D11::wait()
{
	context->Wait(dx_fence.Get(), fence->getValue());
}

void TextureSharedD3D11::signal()
{
	fence->incrementValue();
	context->Signal(dx_fence.Get(), fence->getValue());
}

DXGI_FORMAT TextureSharedD3D11::convert_texture_format() const
{
	switch (texture->getFormat())
	{
		case Texture::FORMAT_R8: return DXGI_FORMAT_R8_UNORM;
		case Texture::FORMAT_RG8: return DXGI_FORMAT_R8G8_UNORM;
		case Texture::FORMAT_RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;

		case Texture::FORMAT_R16: return DXGI_FORMAT_R16_SNORM;
		case Texture::FORMAT_RG16: return DXGI_FORMAT_R16G16_SNORM;
		case Texture::FORMAT_RGBA16: return DXGI_FORMAT_R16G16B16A16_SNORM;

		case Texture::FORMAT_R16U: return DXGI_FORMAT_R16_UNORM;
		case Texture::FORMAT_RG16U: return DXGI_FORMAT_R16G16_UNORM;
		case Texture::FORMAT_RGBA16U: return DXGI_FORMAT_R16G16B16A16_UNORM;

		case Texture::FORMAT_R32U: return DXGI_FORMAT_R32_UINT;
		case Texture::FORMAT_RG32U: return DXGI_FORMAT_R32G32_UINT;
		case Texture::FORMAT_RGBA32U: return DXGI_FORMAT_R32G32B32A32_UINT;

		case Texture::FORMAT_R16F: return DXGI_FORMAT_R16_FLOAT;
		case Texture::FORMAT_RG16F: return DXGI_FORMAT_R16G16_FLOAT;
		case Texture::FORMAT_RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;

		case Texture::FORMAT_R32F: return DXGI_FORMAT_R32_FLOAT;
		case Texture::FORMAT_RG32F: return DXGI_FORMAT_R32G32_FLOAT;
		case Texture::FORMAT_RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;

		case Texture::FORMAT_RGB565: return DXGI_FORMAT_B5G6R5_UNORM;
		case Texture::FORMAT_RGBA4: return DXGI_FORMAT_B4G4R4A4_UNORM;
		case Texture::FORMAT_RGB5A1: return DXGI_FORMAT_B5G5R5A1_UNORM;
		case Texture::FORMAT_RGB10A2: return DXGI_FORMAT_R10G10B10A2_UNORM;
		case Texture::FORMAT_RG11B10F: return DXGI_FORMAT_R11G11B10_FLOAT;
	}

	return DXGI_FORMAT_UNKNOWN;
}
