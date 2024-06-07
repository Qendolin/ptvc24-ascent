#version 430

const int SHADOW_CASCADE_COUNT = 4;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_tangent;
layout(location = 3) in vec2 in_uv;
layout(location = 4) in mat4 in_model_mat;

layout(location = 0) out vec3 out_position_ws; // world space
layout(location = 1) out vec3 out_position_vs; // world space
layout(location = 2) out vec2 out_uv; 
layout(location = 3) out mat3 out_tbn; // uses location 2-4 because of mat3
layout(location = 6) out vec3 out_shadow_position[SHADOW_CASCADE_COUNT]; // shadow ndc space
layout(location = 10) out vec3 out_shadow_direction;

uniform mat4 u_view_mat;
uniform mat4 u_projection_mat;

layout(binding = 6) uniform sampler2DArrayShadow u_shadow_map;
uniform mat4 u_shadow_view_mat[SHADOW_CASCADE_COUNT];
uniform mat4 u_shadow_projection_mat[SHADOW_CASCADE_COUNT];
uniform float u_shadow_normal_bias;

out gl_PerVertex {
	vec4 gl_Position;
};

vec3 shadowSamplePosition(in mat4 view, in mat4 projection, vec3 position, vec3 normal, vec3 direction, float texel_size) {
	vec4 shadow_ws = vec4(position, 1.0);

	// https://web.archive.org/web/20160602232409if_/http://www.dissidentlogic.com/old/images/NormalOffsetShadows/GDC_Poster_NormalOffset.png
	// https://github.com/TheRealMJP/Shadows/blob/8bcc4a4bbe232d5f17eda5907b5a7b5425c54430/Shadows/Mesh.hlsl#L716C8-L716C26
	// https://c0de517e.blogspot.com/2011/05/shadowmap-bias-notes.html
	float n_dot_l = dot(normal, direction);
    // texel_size just help keeping the bias consistent between different resolutions
    vec3 offset = u_shadow_normal_bias * (1.0 - n_dot_l) * texel_size * normal;
	shadow_ws.xyz += offset;

	vec4 shadow_ndc = projection * view * shadow_ws;

	// Usually this divide is required, but the shadow projection is orthogonal, so we can omit it.
	// If this wasn't the case we also couldn't do it in the vs, because vs outputs need to be linear in order to be
	// interpolated properly. 
	// shadow_ndc.xyz / shadow_ndc.w;
	return shadow_ndc.xyz;
}


void main() {
	vec4 position_ws = in_model_mat * vec4(in_position, 1.0);
	vec4 position_vs = u_view_mat * position_ws;
	vec4 position_cs = u_projection_mat * position_vs;
	gl_Position = position_cs;
	
	out_position_vs = position_vs.xyz;
	out_position_ws = position_ws.xyz;
	out_uv = in_uv;
	// NOTE: Non uniform scaling not supported!
	mat3 normal_matrix = mat3(in_model_mat);
	vec3 T = normalize(normal_matrix * in_tangent.xyz);
	vec3 N = normalize(normal_matrix * in_normal);
	vec3 bitangent = cross(in_normal, in_tangent.xyz) * in_tangent.w;
	vec3 B = normalize(normal_matrix * bitangent);
	out_tbn = mat3(T, B, N);

	out_shadow_direction = normalize(transpose(mat3(u_shadow_view_mat[0])) * vec3(0, 0, -1));
	for(int i = 0; i < SHADOW_CASCADE_COUNT; i++) {
		out_shadow_position[i] = shadowSamplePosition(
				u_shadow_view_mat[i], 
				u_shadow_projection_mat[i], 
				out_position_ws,
				N, 
				out_shadow_direction,
				1.0 / textureSize(u_shadow_map, 0).x);
	}
}