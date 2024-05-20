#version 450
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

//out float height;
//out vec3 Position;
out vec2 TexCoord;

//uniform mat4 u_projection_mat;

out gl_PerVertex
{
  vec4 gl_Position;
};

//TODO: add to build and compile shader, include the tes and tcs
void main()
{
    //height = aPos.y;
    //Position = (view * model * vec4(aPos, 1.0)).xyz;
    //gl_Position = u_projection_mat * vec4(aPos, 1);
    TexCoord = aTex;
    gl_Position = vec4(aPos, 1);
}

