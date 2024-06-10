# ptvc24-ascent

When extracting the assets.zip archive make sure the directory strucucture is as follows:

```
/Ascent.exe
/README.md
/LICENSE
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

| Key          | Action                     |
| ------------ | -------------------------- |
| `Shift`, `W` | Boost                      |
| `Space`, `S` | Air Break                  |
| `R`          | Respawn at last Checkpoint |
| `ESC`        | Pause Menu                 |
| `F2`         | Hide GUI                   |
| `F3`         | Toggle debug menu          |
| `F5`         | Reload assets / shaders    |

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
See [docs/Rendering.md](./docs/Rendering.md)

- Physically Based Shading  
https://google.github.io/filament/Filament.html#materialsystem  
https://learnopengl.com/PBR/Theory

- Reversed Depth, Infinite Projection Matrix  
https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/  
https://web.archive.org/web/20220916052250/http://dev.theomader.com/depth-precision/  
https://developer.nvidia.com/content/depth-precision-visualized  
https://www.danielecarbone.com/reverse-depth-buffer-in-opengl/  
https://www.terathon.com/gdc07_lengyel.pdf

- Blender as Level Editor  
Objects can be placed and configured in blender.

## Other Features

### AgX display mapping (tone mapping)
https://www.shadertoy.com/view/dtSGD1  
https://www.shadertoy.com/view/cd3XWr  
https://github.com/sobotka/AgX  

Also used by blender (4.0+).

## Gameplay Featues

- 3D Geometry  
The game can load 3D geometry from glTF files.
Supports instancing.
See [Rendering.md](Rendering.md)

- Playable  
Self explanatory

- 60 FPS  
On the tested system configuration the average is above 200fps.

- Win/Lose  
Since this is a racing game there is no direct "loose" condition.
But you race against your own best time.
Upon reaching the goal a performance graph is shown and the previous hi-score.

- Intuitive Controls  
Controls mimic flying with the Elytra in Minecraft.

- Intuitive Camera  
Just using the usual mouse look cotrols.

- Illumination Model  
Currently using the environment HDRI for lighting (Image Based Lighting).
The object materials are loaded from the glTF file.

- Textures  
Textures are loaded from the glTF file.

- Moving Objects  
Some obstacles move, aswell as the checkpoint propellers.

- Documentation  
See "docs" folder.

- Adjustable Parameters  
The window size is adjustable as well as the FOV and mouse sensitivity.

- Advanced Gameplay  
The game features multiple mechanics and objects that shape the gameplay.

- Collision Detection  
Usig the Jolt physics engine.

- Advanced Physics  
Everything has physics colliders. There is a stack of boxes that can be knocked down the montain.

- Heads-Up Display  
Includes a race timer and a boost meter.
Implemented using the Nuklear library with a custom renderer backend.

## Graphics Effects

- Shadow Map with PCF
Cascased shadow mapping with [4-sample PCF](https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing) and [Normal Offset Biasing](https://web.archive.org/web/20160602232409if_/http://www.dissidentlogic.com/old/images/NormalOffsetShadows/GDC_Poster_NormalOffset.png)

- GPU Particle System using Compute Shader  
A highly customizable Particle System running enitely on the GPU. Many particle properties can be edited at runtime in the debug menu. Mostly based upon [juandiegomontoya](https://juandiegomontoya.github.io/particles.html).

- Tessellation from Height Map  
The montain uses tesselation and has collision.

- Hierarchical Animation  
The propellers on the wodden obstacles rotate while the obstacle moves up and down.

- Vertex Shader Animation  
The water uses animated hieght maps to achive geometric waves. It also uses tesselation.

- Specular Map  
Using specular maps from the glTF file.

- Environment Map  
Skybox and Image Based Lighting.

- Image Based Lighting  
Using a custom `.iblenv` file format.  
Reference: https://github.com/Qendolin/advanced-gl/tree/master/Project03/ibl  
Generated using: https://github.com/Qendolin/advanced-gl/tree/master/Project03/cmd/iblconv  
See [docs/Environment.md](./docs/Environment.md)  
Implementation is based upon
[Fillament](https://google.github.io/filament/Filament.html)
and [learnopengl](https://learnopengl.com/PBR/IBL/Diffuse-irradiance).

- Physically Based Shading  
Note: Wendelin has implemented this in ECG before  
Based on [learnopengl](https://learnopengl.com/PBR/Theory)

- Simple Normal Mapping  
Using tangents and normal maps from the glTF file.  
Open the debug menu (F3) to toggle it on and off.

- Physically Based Bloom/Glow  
Configurable in the debug menu (F3).  
Baed on [CoD: Advanced Warefare](https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/), [learnopengl](https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom) and [Froyok's Blog](https://www.froyok.fr/blog/2021-12-ue4-custom-bloom/).

- Lens Flares, Glare and Light Streaks   
Configurable in the debug menu (F3).  
Based on John Chapman's [Pseudo Lens Flare](https://john-chapman-graphics.blogspot.com/2013/02/pseudo-lens-flare.html),
[this blog post](https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/) and Kawase's ligh streaks from [this presentation](https://www.chrisoat.com/papers/Oat-ScenePostprocessing.pdf).

- Ambient Occlusion  
Using the state of the art [Ground Truth Ambient Occlusion](https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf) algorithm that takes inspiration from Horizon Based Ambient Occlusion aswell as Scalable Ambient Obscurance. It was very difficult to implement due to the lack of resources.

- Motion Blur  
Framerate independent motion blur based on [John Chapman](https://john-chapman-graphics.blogspot.com/2013/01/what-is-motion-blur-motion-pictures-are.html). Note **IT IS DISABLED BY DEFAULT** and can be enabled in the game settings.

## Non Graded Effects

- Volumetric Altitude Fog  
Compared to simple distance based fog this variant takes into account the integral along the view ray to give a much nicer result.

- Dithering  
Uncommon these days but when converting HDR to SDR dithering can still often improve the visual quality.

- Sound and Music
Using to Soloud library. Supports 2d and 3d sounds.

- FPS Limit  
For a smooth gameplay experience FPS can be limited to a fixed number. 120 by default.

- Compute-based Post Processing  
Gives improved performance over rendering a full-screen quad.

- Multithreaded loading  
The biggest assets are loaded asnychronously, which is not so easy in OpengL.

## Debug Menu
Open / Close with F3.  
Contains the option to enable "Free Cam" which will let you fly around freely. The flying speed can be changed using the mouse wheel.  
Also contains many options for the various graphical effects.  
Window placement is not saved in release builds.  

## Libraries

- Tweeny · [GitHub](https://github.com/mobius3/tweeny)  
  Used for UI animation tweens.

- GLM · [GitHub](https://github.com/g-truc/glm)  
  Used for 3D math.

- Nuklear · [GitHub](https://github.com/Immediate-Mode-UI/Nuklear)  
  Used for GUIs.  
  *Note*: Using a custom renderer backend, the library doesn't do any rendering by itself.

- Dear ImGUI · [GitHub](https://github.com/ocornut/imgui)  
  Used for debug GUIs.  
  *Note*: Using a custom renderer backend, the library doesn't do any rendering by itself.

- STB Libraries · [GitHub](https://github.com/nothings/stb)  
  Used for loading images, baking and packing ttf fonts.

- Tiny glTF · [GitHub](https://github.com/syoyo/tinygltf)  
  Used for parsing glTF files.  
  *Note*: Doesn't generate GPU ready objects. Only does parsing.

- Tortellini · [GitHub](https://github.com/Qix-/tortellini)  
  Used for parsing .ini files.

- Jolt · [GitHub](https://github.com/jrouwe/JoltPhysics)  
  Used for 3D physics.

- LZ4 · [GitHub](https://github.com/lz4/lz4)  
  Used for decompressing .iblenv files.

- SoLoud · [GitHub](https://github.com/jarikomppa/soloud)  
  Used for playing audio.

- DDS Image · [GitHub](https://github.com/spnda/dds_image)  
  Small library for loading DDS images. (Used for heightmap albedo)