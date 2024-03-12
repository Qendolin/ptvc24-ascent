#version 450 core


layout(location = 0) in vec4 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
	vec3 color = in_color.rgb * length(in_uv);
	float light = dot(normalize(in_normal), normalize(vec3(0, 1, -1))) * 0.5 + 0.5;
	light = light * 0.8 + 0.2;
	out_color.rgb = color * light;
  	out_color.a = in_color.a;
}