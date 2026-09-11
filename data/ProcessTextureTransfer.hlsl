// Converts RGBA8 source texture to RGBA8 output buffer (dropping alpha on CPU side).
// Writing 3-byte RGB8 per pixel to a byte-address buffer has alignment issues,
// so we write full RGBA and strip alpha during the CPU memcpy step.

cbuffer Constants : register(b0)
{
	int width;
	int height;
	int2 padding;
};

Texture2D<float4> src_texture : register(t0);
RWByteAddressBuffer dest_buffer : register(u0);

[numthreads(16, 16, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
	if (dtid.x >= (uint)width || dtid.y >= (uint)height)
		return;

	float4 pixel = src_texture[dtid.xy];

	uint r = (uint)(saturate(pixel.x) * 255.0);
	uint g = (uint)(saturate(pixel.y) * 255.0);
	uint b = (uint)(saturate(pixel.z) * 255.0);

	// Pack RGB into 3 bytes. We write the pixel as a 3-byte sequence.
	uint dest_offset = (dtid.y * width + dtid.x) * 3;
	uint aligned_offset = dest_offset & ~3u;
	uint byte_in_dword = dest_offset & 3u;

	// Build a 3-byte value
	uint rgb = r | (g << 8) | (b << 16);

	// Use atomic OR to safely write bytes across DWORD boundaries
	uint mask0 = rgb << (byte_in_dword * 8);
	dest_buffer.InterlockedOr(aligned_offset, mask0);

	// Handle spillover to next DWORD if 3 bytes cross a 4-byte boundary
	if (byte_in_dword > 1)
	{
		uint shift = (4 - byte_in_dword) * 8;
		uint mask1 = rgb >> shift;
		if (mask1 != 0)
			dest_buffer.InterlockedOr(aligned_offset + 4, mask1);
	}
}
