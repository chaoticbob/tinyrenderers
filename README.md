# tinyrenderers

### Single header implemenations of Vulkan and D3D12 renderers

#### Features
 - Single header for Vulkan renderer
 - Single header for D3D12 renderer
 - Texture upload + mipmap generation (better quality resizer coming soon)
 - Simplified API shared between both renderers
 - C style structs
 - Support for Vulkan layers
 - Configurable swapchain multi-sample
 - Configurable swapchain imagecount
 - Configurable swapchain depth/stencil attachment
 - Samples use GLFW
   - GLFW
     - Works for both Vulkan and D3D12 - renderer takes over after window handle is obtained
     - Image loading done via [stb_image](https://github.com/nothings/stb)
 - Includes basic compute samples
 - Uses CMake 
 - ...more to come soon

#### Notes
 - Linux and Windows only for the moment
   - Tested on open source intel driver on Linux
   - Tested on AMD and NVIDIA drivers on Windows
   - For best results, use latest drivers
 - Vulkan renderer will work with C/C++
   - For best results, use latest version of Vulkan SDK
 - D3D12 render requires C++
 - Microsoft's C compiler doesn't support certain C11/C99 features, such as VLAs (so alot of awkward array handling)
 - tinyvk/tinydx is written for experimentation and fun-having - not performance
 - For simplicity, only one descriptor set can be bound at once
   - In D3D12, this means two descriptor heaps (CBVSRVUAVs and samplers)
   - For Vulkan shaders the 'set' parameter for 'layout' should always be 0
   - For D3D12 shaders the 'space' parameter for resource bindings should always be 0
 - Vulkan like idioms are used primarily with some D3D12 wherever it makes sense
 - For Vulkan, host visible means both HOST VISIBLE and HOST COHERENT
 - Bring your own math libraary
 - Development was done on Cinder but renderers are not limited to it

#### Compiling and Linking
In one C/C++ file that #includes the renderer's header file, do this:
```
#define TINY_RENDERER_IMPLEMENTATION
#include "tinyvk.h"
```
or
```
#define TINY_RENDERER_IMPLEMENTATION
#include "tinydx.h"
```
#### Building on Linux
```
git clone --recursive git@github.com:chaoticbob/tinyrenderers.git
cd tinyrenderers
mkdir build
cd build
cmake ..
make
```

#### Building on Windows
```
git clone --recursive git@github.com:chaoticbob/tinyrenderers.git
cd tinyrenderers
mkdir build
cd build
cmake -G "Visual Studio 14 2015 Win64" ..
```
Open ```tinyrenderers.sln``` and go

#### Screenshots
![](https://github.com/chaoticbob/tinyrenderers/blob/master/screenshots/tr-004.png?raw=true)
![](https://github.com/chaoticbob/tinyrenderers/blob/master/screenshots/tr-003.png?raw=true)
![](https://github.com/chaoticbob/tinyrenderers/blob/master/screenshots/tr-001.png?raw=true)
![](https://github.com/chaoticbob/tinyrenderers/blob/master/screenshots/tr-002.png?raw=true)


### Change Log
[2017/11/12] - Added simple tessellation shader sample. Fixed misc issues with pipeline setup for tessellation.
[2017/11/10] - Added ChessSet demo. Added geometry shader sample. Fixes to depth stencil handling.
[2017/11/10] - Added TexturedCube sample. Updated depth attachment handling on swapchain render pass. Switched out lc_image for stb_image. Added DXC shader build script.
[2017/05/27] - Fixed some annoying buffer state transitions. Added build script for shaders. Updated shader naming convention to be more exact. Added OpaqueArgs and PassingArrays for investigation.<br>
[2017/05/20] - Updated Append/Consume sample for Vulkan. Requires latest glslang. ConstantBuffer also works for both platforms.<br>
[2017/05/13] - Added Linux support. Moved to project files to cmake. Moved glsl shaders to glsl sub directory - forcing HLSL for now.<br>
[2017/04/30] - Clarified shader usage in some sample programs to point out which source they're coming from.<br>
[2017/04/27] - Added ConstantBuffer sample (D3D12 only for now). Updated Vulkan samples to use negative viewport height.<br>
[2017/04/25] - Updated SimpleCompute and StructuredBuffer to work on Vulkan.<br>
[2017/04/24] - Added compute samples (D3D12 only for now). One for simple compute and another for structured buffers.
