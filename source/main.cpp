// Copyright (C), UNIGINE. All rights reserved.

#if defined(_WIN32)
// D3D12 Agility SDK redistributable, deployed under the app's bin/D3D12 folder.
// The path is resolved relative to the executable, which lives in bin/.
// This sample is Windows-only (it relies on Direct3D 11 <-> Direct3D 12 shared resources).
#	define D3D12_AGILITY_SDK_PATH "D3D12"
#endif

#include <UnigineInit.h>
#include <UnigineEngine.h>

#include "AppSystemLogic.h"
#include "AppWorldLogic.h"

using namespace Unigine;

int main(int argc, char **argv)
{
	// UnigineLogic
	AppSystemLogic system_logic;
	AppWorldLogic world_logic;

	// init engine
	Engine::InitParameters init_params;
	init_params.window_title = "UNIGINE Engine: DirectX 11 C++ integration";
	EnginePtr engine(init_params, argc, argv);

	// enter main loop
	engine->main(&system_logic, &world_logic, nullptr);

	return 0;
}
