#version 450 core

layout(location = 0) in vec2 in_position;

layout(location = 0) out vec3 out_clip_pos;
out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = vec4(in_position, 0., 1.);
	out_clip_pos = vec3(in_position, 0);
}