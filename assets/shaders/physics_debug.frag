#version 450 core


layout(location = 0) in vec4 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
	vec3 color = in_color.rgb;

	// From TrianglePixelShader.hlsl
	// Apply procedural pattern based on the uv coordinates
	bvec2 less_half = lessThan(in_uv - floor(in_uv), vec2(0.5, 0.5));
	float darken_factor = (less_half.r != less_half.g) ? 0.5 : 1.0;

	// Fade out checkerboard pattern when it tiles too often
	vec2 dx = dFdx(in_uv), dy = dFdy(in_uv);
	float texel_distance = sqrt(dot(dx, dx) + dot(dy, dy));
	darken_factor = mix(darken_factor, 0.75, clamp(5.0 * texel_distance - 1.5, 0.0, 1.0));
	
	color *= darken_factor;

	float light = dot(normalize(in_normal), normalize(vec3(0, 1, -1))) * 0.5 + 0.5;
	light = light * 0.8 + 0.2;
	out_color.rgb = color * light;
  	out_color.a = in_color.a;
}