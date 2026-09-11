// Copyright (C), UNIGINE. All rights reserved.

#pragma once

#include <d3d11_4.h>
#include <wrl/client.h>

#include <UnigineTextures.h>
#include <UnigineResourceFence.h>
#include <UnigineResourceExternalMemory.h>
#include <UnigineEvent.h>

using Microsoft::WRL::ComPtr;

class TextureSharedD3D11
{
public:
	TextureSharedD3D11() {}
	~TextureSharedD3D11()
	{
		clear();
	}

	bool exportToD3D11(const Unigine::TexturePtr &src);

	void clear();

	ID3D11UnorderedAccessView *getUAV() const { return uav.Get(); }
	ID3D11ShaderResourceView *getSRV() const { return srv.Get(); }
	ID3D11Texture2D *getDXTexture() const { return dx_texture.Get(); }
	ID3D11DeviceContext4 *getContext() const { return context.Get(); }
	ID3D11Device5 *getDevice() const { return device.Get(); }

	Unigine::ResourceFencePtr &getFence() { return fence; }
	Unigine::Event<> &getEventWork() { return event_work; }

	void wait();
	void signal();

	operator const Unigine::TexturePtr &() const { return texture; }
	const Unigine::TexturePtr &getTexture() const { return texture; }

	bool isInitialized() const { return initialized; }

	int getWidth() const { return texture->getWidth(); }
	int getHeight() const { return texture->getHeight(); }

private:
	DXGI_FORMAT convert_texture_format() const;

	bool initialized{};

	Unigine::TexturePtr texture;

	Unigine::ResourceFencePtr fence{};
	Unigine::ResourceExternalMemoryPtr resource_external_memory{};

	Unigine::EventInvoker<> event_work;
	Unigine::EventConnection event_connection;

	ComPtr<ID3D11Device5> device;
	ComPtr<ID3D11DeviceContext4> context;
	ComPtr<ID3D11Texture2D> dx_texture;
	ComPtr<ID3D11Fence> dx_fence;
	ComPtr<ID3D11UnorderedAccessView> uav;
	ComPtr<ID3D11ShaderResourceView> srv;
};
