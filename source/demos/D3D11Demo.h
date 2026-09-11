// Copyright (C), UNIGINE. All rights reserved.

#pragma once

// Base interface for a single Direct3D 11 integration demo. The dispatcher (AppWorldLogic)
// owns exactly one instance at a time, chosen by the selected demo, and drives its
// lifecycle through the loaded world's init/update/shutdown.
class D3D11Demo
{
public:
	virtual ~D3D11Demo() {}

	virtual void init() {}
	virtual void update() {}
	virtual void shutdown() {}
};
