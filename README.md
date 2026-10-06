# Vulkan Engine
An engine for rendering 3D scenes with the Vulkan Graphics API in C++.



## Features
* The Vulkan-Hpp wrapper (https://github.com/KhronosGroup/Vulkan-Hpp)
* Deferred Shading
* Lighting
	* Screen Space Ambient Occlusion
	* Point lights
	* Directional Shadow Mapping
	* Spotlight Shadows
* Imgui (https://github.com/ocornut/imgui)
* SDL2 (https://www.libsdl.org)
* Physics
	* Bullet 3 Physics Integration for static meshes
	* Automated Model loading (with convex hull construction for bullet)
	

## Building on macOS

Requires Homebrew. Install dependencies, then build with CMake:

```sh
brew install cmake molten-vk vulkan-loader sdl2 assimp bullet
cmake -B build
cmake --build build -j 8
./build/vulkanEngine
```

Notes:
* Runs on MoltenVK (Vulkan-on-Metal). The build compiles against the bundled
  Vulkan 1.0.46 headers and links the modern Homebrew Vulkan loader.
* The bundled SDL2 and assimp headers are Windows-era and ABI-incompatible with
  the Homebrew libraries; the CMake build shims the Homebrew headers in front of
  them (see `compat_include` in CMakeLists.txt).
* Assets are symlinked next to the binary; shaders load from the prebuilt
  `.spv` files. If a shader fails MSL conversion at pipeline creation, recompile
  it from source with `glslc --target-env=vulkan1.0` (shaderc).

## Credits
  * Originally based on https://github.com/jherico/Vulkan