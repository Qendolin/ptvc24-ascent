#version 450 core

const int SHADOW_CASCADE_COUNT = 4;
const int MAX_TESS_LEVEL = 8;

layout(quads, equal_spacing, ccw) in;

layout(location = 0) in vec2 in_texture_coord[];

in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

layout(location = 0) out float out_height;
layout(location = 1) out vec3 out_position_ws;
layout(location = 2) out vec3 out_position_vs;
layout(location = 3) out vec2 out_uv;
layout(location = 4) out vec3 out_shadow_position[SHADOW_CASCADE_COUNT]; // shadow ndc space
layout(location = 8) out vec3 out_shadow_direction;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0) uniform sampler2D u_height_map;
uniform mat4 u_view_mat;
uniform mat4 u_projection_mat;
uniform float u_height_scale;

layout(binding = 7) uniform sampler2DArrayShadow u_shadow_map;
uniform mat4 u_shadow_view_mat[SHADOW_CASCADE_COUNT];
uniform mat4 u_shadow_projection_mat[SHADOW_CASCADE_COUNT];
uniform float u_shadow_normal_bias;

// see pbr.vert
vec3 shadowSamplePosition(in mat4 view, in mat4 projection, vec3 position, vec3 normal, vec3 direction, float texel_size) {
	vec4 shadow_ws = vec4(position, 1.0);
	// float n_dot_l = dot(normal, direction);
    // vec3 offset = u_shadow_normal_bias * (1.0 - n_dot_l) * texel_size * normal;
	// shadow_ws.xyz += offset;

	vec4 shadow_ndc = projection * view * shadow_ws;
	return shadow_ndc.xyz;
}

void main()
{
    float s = gl_TessCoord.x;
    float t = gl_TessCoord.y;

    vec2 t00 = in_texture_coord[0];
    vec2 t01 = in_texture_coord[1];
    vec2 t10 = in_texture_coord[2];
    vec2 t11 = in_texture_coord[3];

    // interpolate patch uvs
    vec2 t0 = (t01 - t00) * s + t00;
    vec2 t1 = (t11 - t10) * s + t10;
    vec2 tex_coord = (t1 - t0) * t + t0;

    float lod = MAX_TESS_LEVEL - log2(max(gl_TessLevelInner[0], gl_TessLevelInner[1]));
    out_height = textureLod(u_height_map, tex_coord, 0.0).r;

    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    const vec4 up = vec4(0.0, 1.0, 0.0, 0.0);
    vec4 p0 = (p01 - p00) * s + p00;
    vec4 p1 = (p11 - p10) * s + p10;
    vec4 p_ws = (p1 - p0) * t + p0 + up * out_height * u_height_scale;
    vec4 p_vs = u_view_mat * p_ws;
    vec4 p_cs = u_projection_mat * p_vs;

    out_position_ws = p_ws.xyz;
    out_position_vs = p_vs.xyz;
    out_uv = tex_coord;

    // scuffed
    vec3 normal = vec3(0.0, 1.0, 0.0);

    out_shadow_direction = normalize(transpose(mat3(u_shadow_view_mat[0])) * vec3(0, 0, -1));
	for(int i = 0; i < SHADOW_CASCADE_COUNT; i++) {
		out_shadow_position[i] = shadowSamplePosition(
				u_shadow_view_mat[i], 
				u_shadow_projection_mat[i], 
				out_position_ws,
				normal, 
				out_shadow_direction,
				1.0 / textureSize(u_shadow_map, 0).x);
	}

    gl_Position = p_cs;
}