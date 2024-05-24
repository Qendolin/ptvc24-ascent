#version 450

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D in_view_normals;
layout(binding = 1) uniform sampler2D in_depth;

uniform mat4 u_inverse_projection_mat;
uniform mat4 u_inverse_view_mat;

vec2 sign_not_zero(vec2 v) {
    return fma(step(vec2(0.0), v), vec2(2.0), vec2(-1.0));
}

// Unpacking from octahedron normals
// https://discourse.panda3d.org/t/glsl-octahedral-normal-packing/15233
vec3 unpack_normal(vec2 n) {
  vec3 v = vec3(n.xy, 1.0 - abs(n.x) - abs(n.y));
  if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * sign_not_zero(v.xy);
  return normalize(v);
}

vec3 reconstruct_view_space_position(float depth, vec2 uv) {
    vec2 clip_xy = uv * 2.0 - 1.0;
    vec4 t = u_inverse_projection_mat * vec4(clip_xy, depth, 1.0);
    return t.xyz / t.w;
}

vec3 load_and_reconstruct_view_space_position(vec2 uv) {
    float depth = texture(in_depth, uv).r;
    return reconstruct_view_space_position(depth, uv);
}

void main() {
  discard;
	// vec3 n = unpack_normal(textureLod(in_view_normals, in_uv, 0).xy);
  // out_color.rgb = n * 0.5 + 0.5;

  vec3 p = load_and_reconstruct_view_space_position(in_uv);
  // out_color.rgb = vec3(-p.z / 100.0);
  out_color.rgb = vec3((p.xyz / 100.0) *0.5 + 0.5);

  out_color.a = 1.0;

}