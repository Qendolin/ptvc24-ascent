#version 450
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texture_coord;

layout(location = 0) out vec2 out_texture_coord;

uniform vec3 u_position;

out gl_PerVertex
{
  vec4 gl_Position;
};

void main()
{
    out_texture_coord = in_texture_coord;
    gl_Position = vec4(vec3(in_position.x, 0.0, in_position.y) + u_position, 1);
}

