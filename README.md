# tinyrenderers

### Single header implemenations of Vulkan and D3D12 renderers


#### Notes
 - Windows only for the moment
 - Vulkan renderer will work with C/C++
 - D3D12 render requires C++
 - Microsoft's C compiler doesn't support certain C11/C99 features, such as VLAs (so alot of awkward array handling)
 - tinyvk/tinydx is written for experimentation and fun-having - not performance
 - For simplicity, only one descriptor set can be bound at once
   - In D3D12, this means two descriptor heaps (CBVSRVUAVs and samplers)
     - CBVSRVUAVs root parameter index is by default at 0
     - Samplers root parameter index is by default at 1
   - For Vulkan shaders the 'set' parameter for 'layout' should always be 0
   - For D3D12 shaders the 'space' parameter for resource bindings should always be 0
 - Vulkan like idioms are used primarily with some D3D12 wherever it makes sense

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
