#version 450 core

const int MAX_TESS_LEVEL = 6;

layout(quads, equal_spacing, ccw) in;

layout(location = 0) in vec2 in_texture_coord[];

in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

layout(location = 0) out float out_height;
layout(location = 1) out vec3 out_position_ws;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec2 out_uv;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0) uniform sampler2D u_height_map;
uniform mat4 u_view_projection_mat;
uniform float u_height_scale;

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
    out_height = textureLod(u_height_map, tex_coord, lod).r;

    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    const vec4 up = vec4(0.0, 1.0, 0.0, 0.0);
    vec4 p0 = (p01 - p00) * s + p00;
    vec4 p1 = (p11 - p10) * s + p10;
    vec4 p = (p1 - p0) * t + p0 + up * out_height * u_height_scale;

    out_position_ws = p.xyz;
    out_uv = tex_coord;
    gl_Position = u_view_projection_mat * p;
}