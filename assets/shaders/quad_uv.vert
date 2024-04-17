#version 450 core

layout(location = 0) in vec2 in_position;

layout(location = 0) out vec2 out_uv;
out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = vec4(in_position, 0.0, 1.);
	out_uv = in_position.xy * 0.5 + 0.5;
}