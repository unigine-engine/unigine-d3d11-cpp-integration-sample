// Copyright (C), UNIGINE. All rights reserved.

#pragma once

#include <UniginePlayers.h>
#include <UnigineTextures.h>
#include <UnigineEvent.h>

#include "D3D11Demo.h"
#include "../d3d11/TextureSharedD3D11.h"

// [2] Texture Write: generates procedural texture content directly on the GPU with a Direct3D 11
// compute shader, writing into a texture shared with the engine (used as a mesh albedo).
class TextureWriteDemo : public D3D11Demo
{
public:
	void init() override;
	void update() override;
	void shutdown() override;

private:
	void work();
	void init_resources();
	void release_resources();

	Unigine::PlayerDummyPtr player;
	Unigine::TexturePtr render_texture;

	TextureSharedD3D11 texture;
	Unigine::EventConnection event_connection;

	ComPtr<ID3D11ComputeShader> compute_shader;
	ComPtr<ID3D11Buffer> constant_buffer;

	int width{0};
	int height{0};
	int frame{0};
};
