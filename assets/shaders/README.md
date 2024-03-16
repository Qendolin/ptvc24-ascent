# Coding conventions

- Use explicit vertex shader attribute locations. [more](https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL)#Vertex_shader_attribute_index)  
  e.g: `layout(location = 0) in vec3 in_position;`
- Use explicit fragment shader output locations. [more](https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL)#Fragment_shader_buffer_output)  
  e.g: `layout(location = 0) out vec4 out_color;`
- Use seperable shader programs. [more](https://www.khronos.org/opengl/wiki/Shader_Compilation#Program_pipelines)  
  Don't forget to declare `out gl_PerVertex { vec4 gl_Position; };` inside vertex shaders.
- Use gamma correct colors. Work in linear space. 

# Naming conventions

- Prefix
  - input attributes with `in_`
  - output attributes with `out_`
  - uniforms with `u_`
- Use `camelCase` for function names.
- Use `snake_case` for variable names.
- The local to world space matrix should be called `model` matrix.
- The world to view space matrix should be called `view` matrix.

## Common abbreviations

| Abbr. | Meaning                   |
| ----- | ------------------------- |
| T     | tangent                   |
| B     | bitangent                 |
| N     | normal                    |
| P     | position                  |
| V     | view direction            |
| L     | light direction           |
| R     | reflected light direction |
| I     | illumination              |
| _tex  | texture                   |
| _mat  | matrix                    |
| _pos  | position                  |
| _vec  | vector                    |
| _fac  | factor                    |
| _ws   | world space               |
| _vs   | view space                |
| _proj | projection                |