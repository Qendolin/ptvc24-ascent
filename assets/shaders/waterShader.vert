#version 450 core
layout (location = 0) in vec2 aPos;

out float Height;

uniform float time;
uniform mat4 projectionView;
uniform mat4 view;
layout(binding = 0) uniform sampler2D waterTexture;
out gl_PerVertex
{
  vec4 gl_Position;
};

void main()
{
    vec3 pos = vec3(aPos.x, 0.0, aPos.y);
    pos.y = texture(waterTexture, aPos/1000+time/20).r * 25;
    gl_Position = projectionView * vec4(pos, 1.0);
    Height = pos.y;
}