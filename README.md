# ptvc24-ascent

When extracting the assets.zip archive make sure the directory strucucture is as follows:
```
/Ascent.exe
/README.md
/assets/fonts/...
/assets/models/...
/assets/shaders/...
/assets/textures/...

```

## Requirements

Opengl 4.5

## Controls

Click to focus the window. Escape to release the focus.  
WASD to fly horizontally. Space & Ctrl to fly vertically. Shift to speed up.  
Use mouse to look.  

| Key          | Action            |
| ------------ | ----------------- |
| `Shift`, `W` | Boost             |
| `Space`, `S` | Air Break         |
| `ESC`        | Pause Menu        |
| `F3`         | Toggle debug menu |
| `F5`         | Reload assets     |

## Arguments
- `--enable-compatibility-profile`  
Enables the OpenGL compatability profile

- `--enable-gl-debug`  
Enables the OpenGL debug callback. Always enabled in debug builds.

## Noteworthy Features

- Modern OpenGL  
  - DSA  
  https://www.khronos.org/opengl/wiki/Direct_State_Access
  - Separate Programs / Shader Pipelines  
  https://www.khronos.org/opengl/wiki/Shader_Compilation#Separate_programs
  - Indirect rendering  
  https://www.khronos.org/opengl/wiki/Vertex_Rendering#Indirect_rendering
  - Immutable Storage  
  https://www.khronos.org/opengl/wiki/Buffer_Object#Immutable_Storage  
  - Explicit uniform locations  
  https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL)#Explicit_uniform_location
  - Separate attribute format  
  https://www.khronos.org/opengl/wiki/Vertex_Specification#Separate_attribute_format

- Material Batching  
[Read more](./docs/Rendering.md)

- GLTF Loading  
https://github.com/syoyo/tinygltf

- Jolt Physics  
https://github.com/jrouwe/JoltPhysics

- Physically Based Shading  
https://google.github.io/filament/Filament.html#materialsystem  
https://learnopengl.com/PBR/Theory

- Reversed Depth, Infinite Projection Matrix  
https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/  
https://web.archive.org/web/20220916052250/http://dev.theomader.com/depth-precision/  
https://developer.nvidia.com/content/depth-precision-visualized  
https://www.danielecarbone.com/reverse-depth-buffer-in-opengl/  
https://www.terathon.com/gdc07_lengyel.pdf

## Other Features

### Nuklear UI
https://github.com/Immediate-Mode-UI/Nuklear

Used for menus and ui

### Dear ImGui
https://github.com/ocornut/imgui

Used for debug ui

### AgX display mapping (tone mapping)
https://www.shadertoy.com/view/dtSGD1  
https://www.shadertoy.com/view/cd3XWr  
https://github.com/sobotka/AgX  

Also used by blender (4.0+).

## Development

When adding .cpp or .h files run `./make rebuild` to rebuild the cmake configuration.
