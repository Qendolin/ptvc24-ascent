## Implemented "Gameplay" Featues

### 3D Geometry
The game can load 3D geometry from glTF files.
Supports instancing.
See [Rendering.md](Rendering.md)

### Playable
Self explanatory

### 60 FPS
On the tested system configuration the average is above 200fps.

### Win/Lose
Since this is a racing game there is no direct "loose" condition.
But you race against your own best time.
Upon reaching the goal a performance graph is shown and the previous hi-score.

### Intuitive Controls
Controls mimic flying with the Elytra in Minecraft.

### Intuitive Camera
Just using the usual mouse look cotrols.

### Illumination Model
Currently using the environment HDRI for lighting (Image Based Lighting).
The object materials are loaded from the glTF file.

### Textures
Textures are loaded from the glTF file.

### Moving Objects
Some obstacles move, aswell as the checkpoint propellers.

### Documentation
See "docs" folder.

### Adjustable Parameters
The window size is adjustable as well as the FOV and mouse sensitivity.

### Collision Detection
Usig the Jolt physics engine.

### Heads-Up Display
Includes a race timer and a boost meter.
Implemented using the Nuklear library with a custom renderer backend.

## Implemented "Effects" Features

### Hierarchical Animation
The propellers on the checkpoints. (I understand if this doesn't count)

### Specular Map
Using specular maps from the glTF file.

### Environment Map
Skybox and Image Based Lighting.

### Image Based Lighting
Using a custom `.iblenv` file format.

Reference: https://github.com/Qendolin/advanced-gl/tree/master/Project03/ibl  
Generated using: https://github.com/Qendolin/advanced-gl/tree/master/Project03/cmd/iblconv  
See [Environment.md](./Environment.md)

Implementation is based upon
[Fillament](https://google.github.io/filament/Filament.html)
and [learnopengl](https://learnopengl.com/PBR/IBL/Diffuse-irradiance).

### Physically Based Shading 
Note: Wendelin has implemented this in ECG before

Based on [learnopengl](https://learnopengl.com/PBR/Theory)

## Planned "Effects" Features

- Shadow Map with PCF
- GPU Particle System using Compute Shader
- Tessellation from Height Map
- Simple Normal Mapping
- PB Bloom/Glow
- Ambient Occlusion (maybe)
- Motion Blur (maybe)