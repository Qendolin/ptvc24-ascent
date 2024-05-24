#version 450

layout(location = 0) in float in_height;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec4 out_color;

void main()
{
    vec3 normal = normalize(in_normal);
    vec3 light_dir = normalize(vec3(1, 1, 0));
    float light = max(dot(normal, light_dir), 0.1);
    out_color.rgb = light * mix(vec3(1.0), vec3(0.474, 0.133, 0.0), 1.0 - pow(1.0 - in_height, 2.0));
    out_color.a = 1.0;
}