// Copyright (C), UNIGINE. All rights reserved.

#ifndef __APP_WORLD_LOGIC_H__
#define __APP_WORLD_LOGIC_H__

#include <UnigineLogic.h>

class D3D11Demo;

// Dispatcher world logic: on each world load it builds the Direct3D 11 demo selected by
// AppSystemLogic (g_selected_demo) and forwards the world lifecycle to it. Switching
// demos is a plain world reload, which tears down one demo's D3D11 state and builds
// the next through the engine's normal world load/unload flow.
class AppWorldLogic : public Unigine::WorldLogic
{
public:
	AppWorldLogic();
	~AppWorldLogic() override;

	int init() override;
	int update() override;
	int shutdown() override;

private:
	D3D11Demo *demo = nullptr;
};

#endif // __APP_WORLD_LOGIC_H__
