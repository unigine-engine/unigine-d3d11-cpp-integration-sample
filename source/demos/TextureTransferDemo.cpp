// Copyright (C), UNIGINE. All rights reserved.

#include "TextureTransferDemo.h"

#include <UnigineEngine.h>
#include <UnigineGame.h>
#include <UnigineLights.h>
#include <UnigineObjects.h>
#include <UnigineMaterials.h>
#include <UnigineMaterial.h>
#include <UnigineRender.h>
#include <UnigineProfiler.h>
#include <UnigineWindowManager.h>
#include <UnigineCallback.h>
#include <UnigineMathLib.h>

#include "../d3d11/D3D11Utils.h"

using namespace Unigine;
using namespace Math;

// process texture on D3D11, with changing size (rgba8 -> rgb8 in sample)
#define PROCESS_TEXTURE true

void TextureTransferDemo::init()
{
	WindowManager::getMainWindow()->setResizable(false);

	// create player
	player = PlayerDummy::create();
	Game::setPlayer(player);

	// create lights
	LightOmniPtr light_0 = LightOmni::create(vec4(1.0f, 0.0f, 0.0f, 1.0f), 100.0f);
	light_0->setIntensity(50.0f);
	light_0->setTransform(translate(Vec3(20.0f, 0.0f, 20.0f)));

	LightOmniPtr light_1 = LightOmni::create(vec4(0.0f, 1.0f, 0.0f, 1.0f), 100.0f);
	light_1->setIntensity(50.0f);
	light_1->setTransform(translate(Vec3(0.0f, 20.0f, 20.0f)));

	LightOmniPtr light_2 = LightOmni::create(vec4(0.0f, 0.0f, 1.0f, 1.0f), 100.0f);
	light_2->setIntensity(50.0f);
	light_2->setTransform(translate(Vec3(-20.0f, 0.0f, 20.0f)));

	// load material
	MaterialPtr mesh_base = Materials::findManualMaterial("Unigine::mesh_base");

	// create meshes (asset shipped in the sample's data/)
	ObjectMeshStaticPtr mesh_0 = ObjectMeshStatic::create("cbox.mesh");
	mesh_0->setTransform(translate(Vec3(-2.0f, 0.0f, 0.0f)));
	mesh_0->setMaterial(mesh_base, "*");

	ObjectMeshStaticPtr mesh_1 = ObjectMeshStatic::create("cbox.mesh");
	mesh_1->setTransform(translate(Vec3(2.0f, 0.0f, 0.0f)));
	mesh_1->setMaterial(mesh_base, "*");

	ObjectMeshStaticPtr mesh_2 = ObjectMeshStatic::create("cbox.mesh");
	mesh_2->setTransform(translate(Vec3(0.0f, -2.0f, 0.0f)));
	mesh_2->setMaterial(mesh_base, "*");

	ObjectMeshStaticPtr mesh_3 = ObjectMeshStatic::create("cbox.mesh");
	mesh_3->setTransform(translate(Vec3(0.0f, 2.0f, 0.0f)));
	mesh_3->setMaterial(mesh_base, "*");

	image = Image::create();
	render_texture = Texture::create();

	Render::getEventEndScreen().connect(event_connection, this, &TextureTransferDemo::transfer_screen_color);

	Profiler::setValue("Engine Transfer", "ms", 0.0f, 30.0f, Math::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	init_resources();
}

void TextureTransferDemo::update()
{
	float time = Game::getTime();
	float x = sinf(time * 1.0f) * 4.0f;
	float y = cosf(time * 1.0f) * 4.0f;
	float z = 2.0f + sinf(time * 3.0f) * 1.0f;
	player->setWorldTransform(setTo(Vec3(x, y, z), Vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)));
	player->setFov(60.0f + sinf(time * 2.0f) * 25.0f);
}

void TextureTransferDemo::transfer_screen_color()
{
	int profiler_micro_id = Profiler::beginMicro("D3D11TextureTransfer", 1);

	// copy screen color
	RenderTargetPtr render_target = Render::getTemporaryRenderTarget();
	render_target->bindColorTexture(0, render_texture);
	render_target->enable();
	{
		Render::renderScreenMaterial("Unigine::render_copy_2d", Renderer::getTextureColor());
	}
	render_target->disable();
	render_target->unbindColorTextures();
	Render::releaseTemporaryRenderTarget(render_target);

	// Try transferring with engine default method
	int index = Engine::get()->getFrame() % NUM_TIME_FRAMES;
	frame_timer[index].begin();
	Render::transferTextureToImage(MakeCallback([this, index](ImagePtr img)
	{
		image->copy(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
		Profiler::setValue("Engine Transfer", "ms", frame_timer[index].endMilliseconds(), 30.0f, Math::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	}), render_texture);

	Profiler::begin("D3D11 Transfer", Math::vec4(0.7f, 0.9f, 0.0f, 1.0f));

	auto *ctx = texture.getContext();

	if (PROCESS_TEXTURE)
	{
		// Clear the output buffer (needed because we use InterlockedOr)
		UINT clear_values[4]{ 0, 0, 0, 0 };
		ctx->ClearUnorderedAccessViewUint(process_buffer_uav.Get(), clear_values);

		// Update constant buffer
		D3D11_MAPPED_SUBRESOURCE mapped;
		DX_CHECK(ctx->Map(constant_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
		struct
		{
			int w;
			int h;
			int pad[2];
		} constants{ width, height, { 0, 0 } };
		memcpy(mapped.pData, &constants, sizeof(constants));
		ctx->Unmap(constant_buffer.Get(), 0);

		// Dispatch compute shader: reads shared texture SRV, writes to process buffer UAV
		ctx->CSSetShader(compute_shader.Get(), nullptr, 0);
		ID3D11Buffer *cbs[]{ constant_buffer.Get() };
		ctx->CSSetConstantBuffers(0, 1, cbs);
		ID3D11ShaderResourceView *srvs[]{ texture.getSRV() };
		ctx->CSSetShaderResources(0, 1, srvs);
		ID3D11UnorderedAccessView *uavs[]{ process_buffer_uav.Get() };
		ctx->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

		ctx->Dispatch((width + 15) / 16, (height + 15) / 16, 1);

		// Unbind
		ID3D11ShaderResourceView *null_srvs[]{ nullptr };
		ctx->CSSetShaderResources(0, 1, null_srvs);
		ID3D11UnorderedAccessView *null_uavs[]{ nullptr };
		ctx->CSSetUnorderedAccessViews(0, 1, null_uavs, nullptr);

		// Copy process buffer to staging buffer for CPU readback
		ctx->CopyResource(staging_buffer.Get(), process_buffer.Get());
		ctx->Flush();

		// Map staging buffer and copy to image
		D3D11_MAPPED_SUBRESOURCE staging_mapped;
		DX_CHECK(ctx->Map(staging_buffer.Get(), 0, D3D11_MAP_READ, 0, &staging_mapped));
		memcpy(image->getPixels(), staging_mapped.pData, image_size);
		ctx->Unmap(staging_buffer.Get(), 0);
	} else
	{
		// Simple path: copy shared texture to staging texture and read back
		ctx->CopyResource(staging_texture.Get(), texture.getDXTexture());
		ctx->Flush();

		D3D11_MAPPED_SUBRESOURCE staging_mapped;
		DX_CHECK(ctx->Map(staging_texture.Get(), 0, D3D11_MAP_READ, 0, &staging_mapped));

		// Copy row by row (staging texture may have different pitch)
		const unsigned char *src = reinterpret_cast<const unsigned char *>(staging_mapped.pData);
		unsigned char *dst = reinterpret_cast<unsigned char *>(image->getPixels());
		size_t row_bytes = static_cast<size_t>(width) * pixel_size;
		for (int y = 0; y < height; y++)
			memcpy(dst + y * row_bytes, src + y * staging_mapped.RowPitch, row_bytes);

		ctx->Unmap(staging_texture.Get(), 0);
	}

	Profiler::end();

	Profiler::endMicro(profiler_micro_id);
}

void TextureTransferDemo::init_resources()
{
	width = WindowManager::getMainWindow()->getSize().x;
	height = WindowManager::getMainWindow()->getSize().y;
	buffer_size = width * height * pixel_size;
	buffer_process_size = width * height * pixel_process_size;

	if (PROCESS_TEXTURE)
	{
		image_format = Image::FORMAT_RGB8;
		image_size = buffer_process_size;
	} else
	{
		image_format = Image::FORMAT_RGBA8;
		image_size = buffer_size;
	}

	render_texture->create2D(width, height, Texture::FORMAT_RGBA8, Texture::FORMAT_USAGE_RENDER | Texture::FORMAT_USAGE_UNORDERED_ACCESS | Texture::FORMAT_USAGE_SHARED);
	texture.exportToD3D11(render_texture);

	auto *dev = texture.getDevice();

	if (PROCESS_TEXTURE)
	{
		// Compile compute shader
		compute_shader = compileComputeShader(dev, "ProcessTextureTransfer.hlsl");

		// Create constant buffer
		D3D11_BUFFER_DESC cb_desc{};
		cb_desc.ByteWidth = 16;
		cb_desc.Usage = D3D11_USAGE_DYNAMIC;
		cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		DX_CHECK(dev->CreateBuffer(&cb_desc, nullptr, &constant_buffer));

		// Create GPU process buffer (for compute shader output)
		D3D11_BUFFER_DESC buf_desc{};
		buf_desc.ByteWidth = buffer_process_size;
		buf_desc.Usage = D3D11_USAGE_DEFAULT;
		buf_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		buf_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		DX_CHECK(dev->CreateBuffer(&buf_desc, nullptr, &process_buffer));

		// Create UAV for process buffer
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
		uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		uav_desc.Buffer.FirstElement = 0;
		uav_desc.Buffer.NumElements = (buffer_process_size + 3) / 4;
		uav_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
		DX_CHECK(dev->CreateUnorderedAccessView(process_buffer.Get(), &uav_desc, &process_buffer_uav));

		// Create staging buffer for CPU readback
		D3D11_BUFFER_DESC staging_desc{};
		staging_desc.ByteWidth = buffer_process_size;
		staging_desc.Usage = D3D11_USAGE_STAGING;
		staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		DX_CHECK(dev->CreateBuffer(&staging_desc, nullptr, &staging_buffer));
	} else
	{
		// Create staging texture for CPU readback
		D3D11_TEXTURE2D_DESC staging_desc{};
		staging_desc.Width = width;
		staging_desc.Height = height;
		staging_desc.MipLevels = 1;
		staging_desc.ArraySize = 1;
		staging_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		staging_desc.SampleDesc.Count = 1;
		staging_desc.Usage = D3D11_USAGE_STAGING;
		staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		DX_CHECK(dev->CreateTexture2D(&staging_desc, nullptr, &staging_texture));
	}

	image->create2D(width, height, image_format);
}

void TextureTransferDemo::release_resources()
{
	texture.clear();

	compute_shader.Reset();
	constant_buffer.Reset();
	process_buffer.Reset();
	process_buffer_uav.Reset();
	staging_buffer.Reset();
	staging_texture.Reset();
}

void TextureTransferDemo::shutdown()
{
	event_connection.disconnect();
	release_resources();
}
