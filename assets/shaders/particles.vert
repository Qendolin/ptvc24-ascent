#version 430

struct Particle {
    // 3f position, 1f rotation
    vec4 position_rotation;
    // 3f velocity, 1f rotation speed
    vec4 velocity_revolutions;
    // 1f drag, 1f gravity
	vec2 drag_gravity;
    // 3f rgb tint, 1f emission
    vec4 tint;
    // 2f size, 1f life remaining, 1f life max
    vec4 size_life;
    int emitter;
};

layout(std430, binding = 0) readonly restrict buffer Particles {
    Particle particles[];
};

layout(location = 0) in vec2 in_position;

uniform mat4 u_projection_mat;
uniform mat4 u_view_mat;
uniform int u_base_index;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_tint;

out gl_PerVertex {
    vec4 gl_Position;
};

const float PI = 3.14159265359;
const float DEG_TO_RAD = PI / 180.0;

void main() {
    // could also use version 460 and gl_BaseInstance
    int index = gl_InstanceID + u_base_index;
    Particle particle = particles[index];

    // https://stackoverflow.com/a/18054309/7448536
    float angle = particle.position_rotation.w * DEG_TO_RAD;
    float cos_angle = cos(angle);
    float sin_angle = sin(angle);
    mat2 rotation = mat2(cos_angle, sin_angle, -sin_angle, cos_angle);
    vec2 local_position = rotation * in_position;

	vec4 view_position = u_view_mat * vec4(particle.position_rotation.xyz, 1.0);
    view_position += vec4(local_position * particle.size_life.xy, 0.0, 0.0);
    gl_Position = u_projection_mat * view_position;

	out_uv = in_position.xy * 0.5 + 0.5;
	out_tint = particle.tint;

	if(particle.size_life.z <= 0.0) {
		// place particle outside the clip space to skip the fragment shader
		gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
	}
}