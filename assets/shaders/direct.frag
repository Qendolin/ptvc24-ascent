#version 450 core


layout(location = 0) in vec3 in_world_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec4 out_color;

uniform vec3 u_camera_position;

void main() {
  float shade = 1.0;
  if(in_normal != vec3(0.0)) {
    vec3 V = normalize(u_camera_position - in_world_position);
    vec3 N = normalize(in_normal);
    shade = abs(dot(N, V));
  }
  out_color = vec4(in_color * shade, 1.);
}