# ptvc24-ascent

## Requirements

Opengl 4.5

## Controls

Click to focus the window. Escape to release the focus.  
WASD to fly horizontally. Space & Ctrl to fly vertically. Shift to speed up.  
Use mouse to look.  

## Arguments
- `--enable-compatibility-profile`  
Enables the OpenGL compatability profile

- `--disable-gl-debug`  
Disables the OpenGL debug callback

## Noteworthy Features

- Modern OpenGL  
  - DSA  
  https://www.khronos.org/opengl/wiki/Direct_State_Access
  - Separate Programs / Shader Pipelines  
  https://www.khronos.org/opengl/wiki/Shader_Compilation#Separate_programs
  - Immutable Storage  
  https://www.khronos.org/opengl/wiki/Buffer_Object#Immutable_Storage  
  - Explicit uniform locations  
  https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL)#Explicit_uniform_location
  - Separate attribute format  
  https://www.khronos.org/opengl/wiki/Vertex_Specification#Separate_attribute_format

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
