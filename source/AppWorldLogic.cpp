// Copyright (C), UNIGINE. All rights reserved.

#include "AppWorldLogic.h"

#include "demos/D3D11Demo.h"
#include "demos/DemoSelection.h"
#include "demos/TextureTransferDemo.h"
#include "demos/TextureWriteDemo.h"

// Selected demo, written by AppSystemLogic before the matching world is loaded.
DemoId g_selected_demo = DemoId::TextureTransfer;

AppWorldLogic::AppWorldLogic()
{}

AppWorldLogic::~AppWorldLogic()
{}

int AppWorldLogic::init()
{
	switch (g_selected_demo)
	{
		case DemoId::TextureWrite:    demo = new TextureWriteDemo(); break;
		case DemoId::TextureTransfer:
		default:                      demo = new TextureTransferDemo(); break;
	}

	demo->init();
	return 1;
}

int AppWorldLogic::update()
{
	if (demo)
		demo->update();
	return 1;
}

int AppWorldLogic::shutdown()
{
	if (demo)
	{
		demo->shutdown();
		delete demo;
		demo = nullptr;
	}
	return 1;
}
