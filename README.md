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
 - ...more to come soon

#### Notes
 - Windows only for the moment
 - Vulkan renderer will work with C/C++
 - D3D12 render requires C++
 - Microsoft's C compiler doesn't support certain C11/C99 features, such as VLAs (so alot of awkward array handling)
 - tinyvk/tinydx is written for experimentation and fun-having - not performance
 - For simplicity, only one descriptor set can be bound at once
   - In D3D12, this means two descriptor heaps (CBVSRVUAVs and samplers)
   - For Vulkan shaders the 'set' parameter for 'layout' should always be 0
   - For D3D12 shaders the 'space' parameter for resource bindings should always be 0
 - Vulkan like idioms are used primarily with some D3D12 wherever it makes sense
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

#### Screenshots
![](https://github.com/chaoticbob/tinyrenderers/blob/master/screenshots/tr-001.png?raw=true)
