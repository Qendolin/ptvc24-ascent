#version 450 core

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D u_albedo_tex;

void main() {
  out_color = vec4(0.0);
  vec3 normal = normalize(in_normal);

  vec4 albedo = texture(u_albedo_tex, in_uv);
  if(albedo.a < 0.5) {
    // because of driver bugs it's good to set it regardless
    out_color = vec4(1.0);
    discard;
  }

  out_color.rgb = pow(albedo.rgb, vec3(1/2.2));
  out_color.a = 1.0;
}