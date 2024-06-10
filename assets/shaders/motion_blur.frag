#version 450 core

// Reference:
// https://john-chapman-graphics.blogspot.com/2013/01/what-is-motion-blur-motion-pictures-are.html
// https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-27-motion-blur-post-processing-effect

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D u_prev_frame_tex;
layout(binding = 1) uniform sampler2D u_depth_tex;

uniform mat4 u_prev_mvp_mat;
uniform float u_motion_blur_scale = 1.0;

uniform mat4 u_inverse_projection_mat;
uniform mat4 u_inverse_view_mat;

vec3 reconstruct_view_space_position(float depth, vec2 uv) {
    vec2 clip_xy = uv * 2.0 - 1.0;
    vec4 t = u_inverse_projection_mat * vec4(clip_xy, depth, 1.0);
    return t.xyz / t.w;
}

vec3 load_and_reconstruct_view_space_position(vec2 uv) {
    float depth = texture(u_depth_tex, uv).r;
    depth = max(depth, 0.000001); // prevent infinity issues 
    return reconstruct_view_space_position(depth, uv);
}


void main() {
	vec3 view_position = load_and_reconstruct_view_space_position(in_uv);
	vec3 world_position = (u_inverse_view_mat * vec4(view_position, 1.0)).xyz;
    vec4 previous = u_prev_mvp_mat * vec4(world_position, 1.0);
    previous.xyz /= previous.w;
    previous.xy = previous.xy * 0.5 + 0.5;

    vec2 blur_vec = previous.xy - in_uv;
    blur_vec *= u_motion_blur_scale;

    const int SAMPLES = 16;

    vec3 result = texture(u_prev_frame_tex, in_uv).rgb;
    for (int i = 1; i < SAMPLES; i++) {
        // get offset in range [-0.5, 0.5]:
        vec2 offset = blur_vec * (float(i) / float(SAMPLES - 1) - 0.5);
    
        // sample & add to result:
        result += texture(u_prev_frame_tex, in_uv + offset).rgb;
    }
    
	result /= float(SAMPLES);
	out_color.rgb = result;
}
