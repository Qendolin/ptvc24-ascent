#version 450 core

const int MAX_TESS_LEVEL = 6;
const float MIN_DISTANCE = 10.0;
const float MAX_DISTANCE = 10000.0;

layout(vertices=4) out;

layout(location = 0) out vec2 out_texture_coord[];

out gl_PerVertex
{
    vec4 gl_Position;
} gl_out[];


layout(location = 0) in vec2 in_texture_coord[];

in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

uniform vec3 u_camera_pos;

void main()
{
    // Pass through positions and texture coordinates
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    out_texture_coord[gl_InvocationID] = in_texture_coord[gl_InvocationID];

    // Control the tessellation levels
    if(gl_InvocationID == 0)
    {
        // Transform positions to eye space
        vec2 eyeSpacePos00 = gl_in[0].gl_Position.xz - u_camera_pos.xz;
        vec2 eyeSpacePos01 = gl_in[1].gl_Position.xz - u_camera_pos.xz;
        vec2 eyeSpacePos10 = gl_in[2].gl_Position.xz - u_camera_pos.xz;
        vec2 eyeSpacePos11 = gl_in[3].gl_Position.xz - u_camera_pos.xz;

        float distance00 = length(eyeSpacePos00);
        float distance01 = length(eyeSpacePos01);
        float distance10 = length(eyeSpacePos10);
        float distance11 = length(eyeSpacePos11);

        // Remap to distance range
        distance00 = max((distance00 - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0);
        distance01 = max((distance01 - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0);
        distance10 = max((distance10 - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0);
        distance11 = max((distance11 - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0);

        // Apply power function, controls falloff
        float tessLevel0 = MAX_TESS_LEVEL * clamp(1.0 - pow(min(distance10, distance00), 0.4), 0.0, 1.0);
        float tessLevel1 = MAX_TESS_LEVEL * clamp(1.0 - pow(min(distance00, distance01), 0.4), 0.0, 1.0);
        float tessLevel2 = MAX_TESS_LEVEL * clamp(1.0 - pow(min(distance01, distance11), 0.4), 0.0, 1.0);
        float tessLevel3 = MAX_TESS_LEVEL * clamp(1.0 - pow(min(distance11, distance10), 0.4), 0.0, 1.0);

        gl_TessLevelOuter[0] = pow(2.0, tessLevel0);
        gl_TessLevelOuter[1] = pow(2.0, tessLevel1);
        gl_TessLevelOuter[2] = pow(2.0, tessLevel2);
        gl_TessLevelOuter[3] = pow(2.0, tessLevel3);

        gl_TessLevelInner[0] = pow(2.0, max(tessLevel1, tessLevel3));
        gl_TessLevelInner[1] = pow(2.0, max(tessLevel0, tessLevel2));
    }
}
