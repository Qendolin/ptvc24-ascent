#version 430

struct Particle {
    // 3f position, 1f rotation
    vec4 position_rotation;
    // 3f velocity, 1f rotation speed
    vec4 velocity_revolutions;
    // 1f drag, 1f gravity, 1f random
    vec4 drag_gravity_rand;
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

uniform float u_emissivity;
uniform float u_stretching;
layout(binding = 1) uniform sampler1DArray u_tint;
layout(binding = 2) uniform sampler2D u_scale;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_tint;
layout(location = 2) out float out_emission;

out gl_PerVertex {
    vec4 gl_Position;
};

const float PI = 3.14159265359;
const float DEG_TO_RAD = PI / 180.0;

void main() {
    // could also use version 460 and gl_BaseInstance
    int index = gl_InstanceID + u_base_index;
    Particle particle = particles[index];
    float life_frac = 1.0 - particle.size_life.z / particle.size_life.w;
    float rand = particle.drag_gravity_rand.z;

    // rotation
    float angle = particle.position_rotation.w * DEG_TO_RAD;
    float cos_angle = cos(angle);
    float sin_angle = sin(angle);
    mat2 rotation = mat2(cos_angle, sin_angle, -sin_angle, cos_angle);
    vec2 local_position = rotation * in_position;
    
    // stretching
    vec3 view_velocity = mat3(u_view_mat) * particle.velocity_revolutions.xyz;
    local_position += dot(normalize(view_velocity.xy), local_position) * view_velocity.xy * u_stretching;

	vec4 view_position = u_view_mat * vec4(particle.position_rotation.xyz, 1.0);

    vec2 scale = particle.size_life.xy * texture(u_scale, vec2(life_frac, rand)).xy;
    view_position += vec4(local_position * scale, 0.0, 0.0);
    
    // https://stackoverflow.com/a/18054309/7448536
    gl_Position = u_projection_mat * view_position;
	out_uv = in_position.xy * 0.5 + 0.5;

    vec2 tint_uv = vec2(life_frac, rand * textureSize(u_tint, 0).y);
	out_tint = texture(u_tint, tint_uv).rgb;
    out_emission = u_emissivity;

	if(particle.size_life.z <= 0.0) {
		// place particle outside the clip space to skip the fragment shader
		gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
	}
}