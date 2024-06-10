#version 450

layout(early_fragment_tests) in;

const int LIGHT_COUNT = 1;

layout(location = 0) in float in_height;
layout(location = 1) in vec3 in_position_ws;
layout(location = 2) in vec3 in_position_vs;
layout(location = 3) in vec2 in_uv;
layout(location = 4) in float in_crest;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_normal;

uniform vec3 u_camera_pos;
uniform float u_near_plane;

layout(binding = 1) uniform sampler2D u_normal_tex;
layout(binding = 2) uniform sampler2D u_depth_tex;
layout(binding = 4) uniform samplerCube u_ibl_diffuse;
layout(binding = 5) uniform samplerCube u_ibl_specualr;
layout(binding = 6) uniform sampler2D u_ibl_brdf_lut;

uniform vec3 u_light_dir[LIGHT_COUNT];
uniform vec3 u_light_radiance[LIGHT_COUNT];

const float PI = 3.14159265359;

// Octahedral Normal Packing
// Credit: https://discourse.panda3d.org/t/glsl-octahedral-normal-packing/15233
// For each component of v, returns -1 if the component is < 0, else 1
vec2 signNotZero(vec2 v) {
    return fma(step(vec2(0.0), v), vec2(2.0), vec2(-1.0));
}

// Packs a 3-component normal to 2 channels using octahedron normals
vec2 packNormal(vec3 n) {
  n.xy /= dot(abs(n), vec3(1));
  return mix(n.xy, (1.0 - abs(n.yx)) * signNotZero(n.xy), step(n.z, 0.0));
}

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a_2 = a*a;
    float n_dot_h = max(dot(N, H), 0.0);
    float n_dot_h_2 = n_dot_h*n_dot_h;

    float nom   = a_2;
    float denom = (n_dot_h_2 * (a_2 - 1.0) + 1.0);
    // when roughness is zero and N = H denom would be 0
    denom = PI * denom * denom + 5e-6;

    return nom / denom;
}

float geometrySchlickGGX(float n_dot_v, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = n_dot_v;
    float denom = n_dot_v * (1.0 - k) + k;

    return nom / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    // + 5e-6 to prevent artifacts, value is from https://google.github.io/filament/Filament.html#materialsystem/specularbrdf:~:text=float%20NoV%20%3D%20abs(dot(n%2C%20v))%20%2B%201e%2D5%3B
    float n_dot_v = max(dot(N, V), 0.0) + 5e-6;
    float n_dot_l = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(n_dot_v, roughness);
    float ggx1 = geometrySchlickGGX(n_dot_l, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cos_theta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cos_theta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 sampleAmbient(vec3 N, vec3 V, vec3 R, vec3 F0, float roughness, vec3 albedo)
{
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    vec3 irradiance = texture(u_ibl_diffuse, N).rgb;
    vec3 diffuse    = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 reflection = textureLod(u_ibl_specualr, R, roughness * MAX_REFLECTION_LOD).rgb;   
    vec2 envBRDF  = texture(u_ibl_brdf_lut, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = reflection * (F * envBRDF.x + envBRDF.y);

    return kD * diffuse + specular; 
}

float linear_depth(float depth) {
    return u_near_plane / depth;
}

void main()
{
    vec2 texel_size = 1.0 / vec2(textureSize(u_depth_tex, 0).xy);
    float back_depth = linear_depth(texelFetch(u_depth_tex, ivec2(gl_FragCoord.xy), 0).r);
    float front_depth = linear_depth(gl_FragCoord.z);
    float depth = back_depth - front_depth;

    vec3 albedo = vec3(25.0, 34.0, 54.0) / 255.0;
    // lighter water color near shore
    albedo = mix(albedo, vec3(75, 163, 222) / 255.0, 1.0 - smoothstep(0.0, 150.0, depth));
    // white crests
    albedo = mix(albedo, vec3(1.0), in_crest);
    float alpha = mix(0.6, 0.8, in_crest);
    // fade out water near the shore
    alpha *= min(depth / 2.0, 1.0);
    float roughness = mix(0.15, 0.4, in_crest);

    vec3 tNx = vec3(texel_size.x, dFdx(in_height), 0.0);
    vec3 tNy = vec3(0.0, dFdy(in_height), texel_size.y);
    vec3 tN = cross(tNx, tNy);
    tN = normalize(tN) * 0.7; // adjust normal strength
    tN.z = sqrt(1-dot(tN.xy, tN.xy));

    vec3 N = tN.xzy;
    vec3 P = in_position_ws;
    vec3 V = normalize(u_camera_pos - P);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.3); // unrealistic value because it looks nice

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < LIGHT_COUNT; ++i)
    {
        vec3 L = normalize(u_light_dir[i]);
        vec3 radiance = u_light_radiance[i];

        // The half way vector
        vec3 H = normalize(V + L);

        // Cook-Torrance BRDF
        float NDF = distributionGGX(N, H, roughness);
        float G   = geometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-5; // + 1e-5 to prevent divide by zero
        vec3 specular = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;

        // scale light by n_dot_l
        float n_dot_l = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * n_dot_l;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // ambient lighting
    vec3 ambient = sampleAmbient(N, V, R, F0, roughness, albedo);
    ambient = min(ambient, 50.0);

    vec3 color = ambient + Lo;
    out_color = vec4(color, alpha);

    out_normal = vec2(0.0);
}