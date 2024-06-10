#version 450 core

const int SHADOW_CASCADE_COUNT = 4;
const int MAX_TESS_LEVEL = 9;

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
layout(location = 4) out float out_crest;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0) uniform sampler2D u_height_map;
uniform mat4 u_view_mat;
uniform mat4 u_projection_mat;
uniform float u_height_scale;
uniform float u_time;

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
    float large_waves = 0.0;
    large_waves += textureLod(u_height_map, tex_coord*1 + u_time*vec2(0,1)/300, lod).r;
    large_waves += textureLod(u_height_map, tex_coord*1.1 + u_time*vec2(1,0)/250, lod).r;
    
    float small_waves = 0.0;
    small_waves += textureLod(u_height_map, tex_coord*8 + u_time*normalize(vec2(1,1))/50, lod).r*0.2;
    small_waves += textureLod(u_height_map, tex_coord*8 + u_time*normalize(vec2(1,3))/30, lod).r*0.2;
    out_crest = smoothstep(0.22, 0.25, small_waves);

    out_height = (1.5 * large_waves + small_waves) * u_height_scale;

    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    const vec4 up = vec4(0.0, 1.0, 0.0, 0.0);
    vec4 p0 = (p01 - p00) * s + p00;
    vec4 p1 = (p11 - p10) * s + p10;
    vec4 p_ws = (p1 - p0) * t + p0 + up * out_height;
    vec4 p_vs = u_view_mat * p_ws;
    vec4 p_cs = u_projection_mat * p_vs;

    out_position_ws = p_ws.xyz;
    out_position_vs = p_vs.xyz;
    out_uv = tex_coord;

    gl_Position = p_cs;
}