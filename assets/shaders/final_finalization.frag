#version 450 core
// great shader name

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec3 out_color;

layout(binding = 0) uniform sampler2D u_color_tex;

// factor, inner radius, outer radius, sharpness
uniform vec4 u_vignette_params;

// https://www.shadertoy.com/view/tt2cDK
float vignette(vec2 uv) {
    float inner = u_vignette_params.y;
    float outer = u_vignette_params.z;
    float sharpness = u_vignette_params.w;

    vec2 curve = pow(abs(uv * 2.0 - 1.0), vec2(sharpness));
    float edge = pow(length(curve), 1.0 / sharpness);
    float vignette = 1.0 - smoothstep(inner, outer, edge);

    return 1.0 - vignette;
}

void main() {
    vec3 color = texture(u_color_tex, in_uv).rgb;

	// Vignette
    color = mix(color, vec3(0.0, 0.0, 0.0), vignette(in_uv) * u_vignette_params.x);

	out_color = color;
}