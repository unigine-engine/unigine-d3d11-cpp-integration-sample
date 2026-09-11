// Copyright (C), UNIGINE. All rights reserved.

#include "TextureWriteDemo.h"

#include <UnigineGame.h>
#include <UnigineLights.h>
#include <UnigineObjects.h>
#include <UnigineMaterials.h>
#include <UnigineMaterial.h>
#include <UnigineProfiler.h>
#include <UnigineMathLib.h>

#include "../d3d11/D3D11Utils.h"

using namespace Unigine;
using namespace Math;

void TextureWriteDemo::init()
{
	// create player
	player = PlayerDummy::create();
	Game::setPlayer(player);

	// create lights
	LightWorldPtr sun = LightWorld::create(Math::vec4_one);

	// create shared texture
	init_resources();

	// load material
	MaterialPtr mesh_base = Materials::findManualMaterial("Unigine::mesh_base");
	mesh_base = mesh_base->inherit();
	mesh_base->setTexture("albedo", render_texture);

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
}

void TextureWriteDemo::update()
{
	float time = Game::getTime();
	float x = sinf(time * 1.0f) * 4.0f;
	float y = cosf(time * 1.0f) * 4.0f;
	float z = 2.0f + sinf(time * 3.0f) * 1.0f;
	player->setWorldTransform(setTo(Vec3(x, y, z), Vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)));
	player->setFov(60.0f + sinf(time * 2.0f) * 25.0f);
}

void TextureWriteDemo::work()
{
	int profiler_micro_id = Profiler::beginMicro("D3D11TextureWrite");
	Profiler::begin("Texture Write");

	auto *ctx = texture.getContext();

	// Update constant buffer
	D3D11_MAPPED_SUBRESOURCE mapped;
	DX_CHECK(ctx->Map(constant_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
	struct { int w; int h; int f; int pad; } constants = {width, height, frame, 0};
	memcpy(mapped.pData, &constants, sizeof(constants));
	ctx->Unmap(constant_buffer.Get(), 0);

	// Dispatch compute shader
	ctx->CSSetShader(compute_shader.Get(), nullptr, 0);
	ID3D11Buffer *cbs[] = {constant_buffer.Get()};
	ctx->CSSetConstantBuffers(0, 1, cbs);
	ID3D11UnorderedAccessView *uavs[] = {texture.getUAV()};
	ctx->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

	ctx->Dispatch((width + 15) / 16, (height + 15) / 16, 1);

	// Unbind and flush
	ID3D11UnorderedAccessView *null_uavs[] = {nullptr};
	ctx->CSSetUnorderedAccessViews(0, 1, null_uavs, nullptr);
	ctx->Flush();

	frame++;

	Profiler::end();
	Profiler::endMicro(profiler_micro_id);
}

void TextureWriteDemo::init_resources()
{
	width = 512;
	height = 512;
	frame = 0;

	render_texture = Texture::create();
	render_texture->create2D(width, height, Texture::FORMAT_RGBA8, Texture::FORMAT_USAGE_RENDER | Texture::FORMAT_USAGE_UNORDERED_ACCESS | Texture::FORMAT_USAGE_SHARED);

	texture.exportToD3D11(render_texture);
	texture.getEventWork().connect(event_connection, this, &TextureWriteDemo::work);

	// Compile compute shader
	compute_shader = compileComputeShader(texture.getDevice(), "ProcessTextureWrite.hlsl");

	// Create constant buffer
	D3D11_BUFFER_DESC cb_desc{};
	cb_desc.ByteWidth = 16;
	cb_desc.Usage = D3D11_USAGE_DYNAMIC;
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DX_CHECK(texture.getDevice()->CreateBuffer(&cb_desc, nullptr, &constant_buffer));
}

void TextureWriteDemo::release_resources()
{
	event_connection.disconnect();
	compute_shader.Reset();
	constant_buffer.Reset();
	texture.clear();
	render_texture = nullptr;
}

void TextureWriteDemo::shutdown()
{
	release_resources();
}
