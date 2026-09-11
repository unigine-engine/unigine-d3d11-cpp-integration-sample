# Direct3D 11 Integration Sample

[**Direct3D 11**](https://learn.microsoft.com/en-us/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11)
can share GPU resources with UNIGINE **without a CPU round-trip**. This sample shows how to share a
texture between the engine and a Direct3D 11 device using the engine's external-memory / fence interop
over **Direct3D 12 shared resources**, so the engine must run on the Direct3D 12 backend
(`-video_app dx12`). One executable bundles two demos; switch between them at runtime with keys **1–2**:

| Key | Demo | What it shows |
|---|---|---|
| **1** | **Texture Transfer** | reads a rendered texture back through a D3D11 compute shader, converting `rgba8 -> rgb8` |
| **2** | **Texture Write** | generates procedural texture content on the GPU with a D3D11 compute shader, used as a mesh albedo |

The shared-resource plumbing lives in `source/d3d11/` (`D3D11Utils` for device / shared-handle
helpers and runtime HLSL compilation, `TextureSharedD3D11` for the engine-texture <-> D3D11
shared-resource wrapper); each demo's compute shader is a separate `.hlsl` file in `data/`.

This is the Direct3D 11 counterpart of the *Texture Transfer* and *Texture Write* demos in the
`unigine-cuda-cpp-integration-sample`.

## How to Run the Sample

### Prerequisites

- [**UNIGINE SDK Browser**](https://developer.unigine.com/en/docs/latest/start/installing_sdk) (latest version)
- **UNIGINE SDK Community** or **Engineering** edition (**Sim** upgrade supported)
- **Visual Studio 2022**
- **Windows 10 version 1703 or later** with **DirectX 11.4** support
- A **Direct3D 12** rendering backend (`-video_app dx12`) — the shared-resource interop is unavailable on other backends

> [!NOTE]
> This sample is **Windows-only**: it shares a texture between the engine's Direct3D 12 device and a
> Direct3D 11 device using Win32 shared handles. There are no Linux project files, and the Linux build
> skips the sample.

### Third-party dependency

None beyond the operating system. The sample links the Windows system libraries `d3d11.lib`, `dxgi.lib`
and `d3dcompiler.lib`; the D3D11 compute shaders (`data/ProcessTextureTransfer.hlsl`,
`data/ProcessTextureWrite.hlsl`) are compiled at runtime.

### Step-by-Step Guide

1. **Clone or download** the sample.
2. **Open SDK Browser** and make sure you have the latest version.
3. **Add the sample project**: *My Projects* → *Add Existing* → select the `.project` file that
   matches your edition and precision (`*_win_*`) → *Import Project*.
4. **Repair** the project (only essential files are in Git; SDK Browser restores the rest), then *Configure Project*.
5. **Open** the project in your IDE: load the folder containing `source/CMakeLists.txt` (Visual Studio 2022 recommended).
6. **Build** and **Run**. Press **1–2** to switch demos.

> [!WARNING]
> Precision must match the `.project` you selected. The coordinate precision is set in
> `source/CMakeLists.txt`:
> ```diff
> - set(UNIGINE_DOUBLE False CACHE BOOL "Double coords")
> + set(UNIGINE_DOUBLE True  CACHE BOOL "Double coords")
> ```
> Use a `*_double.project` for double-precision builds and a `*_float.project` for float.

## What the Sample Contains

```
unigine-d3d11-cpp-integration-sample/
  source/     — main.cpp, AppSystemLogic (legend + 1-2 hotkeys),
                AppWorldLogic (demo dispatcher),
                demos/ (TextureTransfer, TextureWrite),
                d3d11/ (D3D11Utils: device/shared-handle helpers + runtime HLSL compile;
                        TextureSharedD3D11: engine texture <-> D3D11 shared-resource wrapper),
                CMakeLists.txt + cmake/ (Engine resolution)
  data/       — D3D11_texture_transfer / D3D11_texture_write worlds, cbox.mesh,
                ProcessTextureTransfer.hlsl / ProcessTextureWrite.hlsl (D3D11 compute shaders),
                root_mount.umount
  README.md   — this file
  *.project   — SDK Browser project files (Windows, per edition / precision)
```

The whole repository is built by the shared scripts in the repo-root `ci/`
(`build_windows.bat` / `build_linux.sh`); samples do not carry their own `ci/` folder.

## If the Sample Fails to Run

- Re-check every setup step above.
- Make sure the engine runs on the **Direct3D 12** backend (`-video_app dx12`); the shared-resource interop is not available on other backends.
- Ensure the machine has **Windows 10 1703+** with **DirectX 11.4** support.
- Ensure `UNIGINE_DOUBLE` matches the current build type (double/float) and the chosen `.project`.
- Use the `.project` file for your SDK edition.
- Verify your SDK version is not older than the project's specified version.
- C++/CMake issues in Visual Studio: right-click the project → **Delete Cache and Reconfigure**, then rebuild.
