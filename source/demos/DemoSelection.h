// Copyright (C), UNIGINE. All rights reserved.

#pragma once

// Identifies which Direct3D 11 demo the dispatcher should build. AppSystemLogic sets it
// right before triggering the matching world load; AppWorldLogic::init() reads it to
// instantiate the correct demo controller. This keeps demo selection deterministic
// instead of parsing the loaded world's path.
enum class DemoId
{
	TextureTransfer,
	TextureWrite,
};

extern DemoId g_selected_demo;
