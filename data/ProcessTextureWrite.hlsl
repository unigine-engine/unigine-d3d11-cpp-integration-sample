cbuffer Constants : register(b0)
{
	int width;
	int height;
	int frame;
	int padding;
};

RWTexture2D<unorm float4> dest : register(u0);

[numthreads(16, 16, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
	if (dtid.x >= (uint)width || dtid.y >= (uint)height)
		return;

	float4 color;
	color.x = float((dtid.x + frame) & 0xFF) / 255.0;
	color.y = float((dtid.y + frame) & 0xFF) / 255.0;
	color.z = 0.0;
	color.w = 1.0;
	dest[dtid.xy] = color;
}
