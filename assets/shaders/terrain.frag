#version 450

layout(location = 0) in float in_height;
layout(location = 1) in vec3 in_position_ws;
// layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_normal;

uniform vec3 u_camera_pos;
uniform mat4 u_view_mat;
uniform float u_height_scale;
layout(binding = 1) uniform sampler2D u_albedo_tex;
layout(binding = 2) uniform sampler2D u_normal_tex;
layout(binding = 3) uniform sampler2D u_ao_tex;
layout(binding = 4) uniform samplerCube u_ibl_diffuse;
layout(binding = 5) uniform samplerCube u_ibl_specualr;
layout(binding = 6) uniform sampler2D u_ibl_brdf_lut;

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

vec3 sampleAmbient(vec3 N, vec3 V, vec3 R, vec3 F0, float roughness, float metallic, vec3 albedo, float ao)
{
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    vec3 irradiance = texture(u_ibl_diffuse, N).rgb;
    vec3 diffuse    = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 reflection = textureLod(u_ibl_specualr, R, roughness * MAX_REFLECTION_LOD).rgb;   
    vec2 envBRDF  = texture(u_ibl_brdf_lut, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = reflection * (F * envBRDF.x + envBRDF.y);

    return (kD * diffuse + specular) * ao; 
}

void main()
{
    vec3 albedo = texture(u_albedo_tex, in_uv).rgb;
    float ao        = texture(u_ao_tex, in_uv).r;
    float metallic  = 0.0;
    float roughness = 0.9;
    // Note: normal map needs to fit the height scale
    vec3 tN = texture(u_normal_tex, in_uv).xyz * 2.0 - 1.0;

    ao = pow(ao, 1.5);

    vec3 N = normalize((tN).xzy);
    vec3 P = in_position_ws;
    vec3 V = normalize(u_camera_pos - P);
    vec3 R = reflect(-V, N);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    // for(int i = 0; i < ORTHO_LIGHT_COUNT + POINT_LIGHT_COUNT; ++i)
    // Note: currently disabled (just using ibl)
    for(int i = 0; i < 0; ++i)
    {
        vec3 L, radiance;
        L = normalize(vec3(0.0, 1.0, -1.0));
        radiance = vec3(5.0);
        // if(i < ORTHO_LIGHT_COUNT) {
        //     DirectionalLight light = u_directional_lights[i];
        //     L = normalize(-light.direction.xyz);
        //     radiance = light.color.rgb * light.color.a;
        // } else {
        //     PointLight light = u_point_lights[i-ORTHO_LIGHT_COUNT];
        //     L = normalize(light.position.xyz - P);
        //     float d = length(light.position.xyz - P) + 1e-5;
        //     float attenuation = light.attenuation.x + light.attenuation.y * d + light.attenuation.z * d * d;
        //     radiance = (light.color.rgb * light.color.a) / attenuation;
        // }

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
        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;

        // scale light by n_dot_l
        float n_dot_l = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * n_dot_l;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // ambient lighting
    const float shadow_ambient_factor = 0.66;
    vec3 ambient = sampleAmbient(N, V, R, F0, roughness, metallic, albedo, ao);

    vec3 color = ambient + Lo;
    out_color = vec4(color, 1.0);

    // Note: I don't really understand why the inverse view matrix isn't needed here
    out_normal = packNormal(mat3(u_view_mat) * N);
}