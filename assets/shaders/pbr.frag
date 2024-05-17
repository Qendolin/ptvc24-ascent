#version 450 core

layout(early_fragment_tests) in;

layout(location = 0) in vec3 in_position_ws; // world space
layout(location = 1) in vec2 in_uv;
layout(location = 2) in mat3 in_tbn;
layout(location = 5) in vec3 in_shadow_position; // shadow ndc space
layout(location = 6) in vec3 in_shadow_direction;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D u_albedo_tex;
layout(binding = 1) uniform sampler2D u_occlusion_metallic_roughness_tex;
layout(binding = 2) uniform sampler2D u_normal_tex;

layout(binding = 3) uniform samplerCube u_ibl_diffuse;
layout(binding = 4) uniform samplerCube u_ibl_specualr;
layout(binding = 5) uniform sampler2D u_ibl_brdf_lut;

layout(binding = 6) uniform sampler2DShadow u_shadow_map;

uniform vec3 u_camera_pos;
uniform vec3 u_albedo_fac;
uniform vec3 u_occlusion_metallic_roughness_fac;
uniform float u_normal_fac;

uniform float u_shadow_depth_bias;

const float PI = 3.14159265359;

vec3 transformNormal(mat3 tbn, vec3 tangent_normal) {
    return normalize(tbn * tangent_normal);
}

// Based on https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/course-notes-moving-frostbite-to-pbr-v32.pdf page 92
float adjustRoughness(vec3 tangent_normal, float roughness) {
    float r = length(tangent_normal);
    if (r < 1.0) {
        float kappa = (3.0 * r - r * r * r) / (1.0 - r * r);
        float variance = 1.0 / kappa;
        // Why is it ok for the roughness to be > 1 ?
        return sqrt(roughness * roughness + variance);
    }
    return roughness;
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

float sampleShadow(vec3 P_shadow_ndc, float n_dot_l) {
    vec2 texel_size = 1.0 / textureSize(u_shadow_map, 0);
    // z is seperate because we are using 0..1 depth, not the usual -1..1
    vec3 shadow_uvz = vec3(P_shadow_ndc.xy * 0.5 + 0.5, P_shadow_ndc.z);

    // float bias = u_shadow_depth_bias * texel_size.x * tan(acos(n_dot_l));
    float bias = u_shadow_depth_bias * texel_size.x * tan(acos(n_dot_l));
    bias = clamp(bias, 0.0, 0.01);

    // GPU Gems 1 / Chapter 11.4
    vec2 offset = vec2(fract(gl_FragCoord.x * 0.5) > 0.25, fract(gl_FragCoord.y * 0.5) > 0.25); // mod
    offset.y += offset.x; // y ^= x in floating point
    if (offset.y > 1.1) offset.y = 0;
    float shadow = 0.0;
    // + bias instead of - bias becase we are using reversed depth and the GL_GEQUAL compare mode.
    shadow += texture(u_shadow_map, vec3(shadow_uvz.xy + (offset + vec2(-1.5, 0.5)) * texel_size, shadow_uvz.z + bias));
    shadow += texture(u_shadow_map, vec3(shadow_uvz.xy + (offset + vec2(0.5, 0.5)) * texel_size, shadow_uvz.z + bias));
    shadow += texture(u_shadow_map, vec3(shadow_uvz.xy + (offset + vec2(-1.5, -1.5)) * texel_size, shadow_uvz.z + bias));
    shadow += texture(u_shadow_map, vec3(shadow_uvz.xy + (offset + vec2(0.5, -1.5)) * texel_size, shadow_uvz.z + bias));

    return shadow * 0.25;
}

void main()
{
    vec3 albedo = texture(u_albedo_tex, in_uv).rgb             * u_albedo_fac;
    vec3 omr    = texture(u_occlusion_metallic_roughness_tex, in_uv).rbg * u_occlusion_metallic_roughness_fac;
    float ao        = omr.x;
    float metallic  = omr.y;
    float roughness = omr.z;
    // I'm not sure if the normalize is required.
    // When passing just the normal vector from VS to FS it is generally required to normalize it again.
    // See https://www.lighthouse3d.com/tutorials/glsl-12-tutorial/normalization-issues/
    // mat3 tbn = mat3(normalize(in_tbn[0]), normalize(in_tbn[1]), normalize(in_tbn[2]));
    mat3 tbn = in_tbn;

    vec3 tN = texture(u_normal_tex, in_uv).xyz * 2.0 - 1.0;
    tN = normalize(tN * vec3(u_normal_fac, u_normal_fac, 1.0)); // increase intensity
    roughness = adjustRoughness(tN, roughness);

    vec3 N = transformNormal(tbn, tN);
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

    float shadow = sampleShadow(in_shadow_position, dot(tbn[2], in_shadow_direction));

    // ambient lighting
    // TODO: figure out physically based way for shadow contribution to ambient light
    const float shadow_ambient_factor = 0.66;
    vec3 ambient = sampleAmbient(N, V, R, F0, roughness, metallic, albedo, ao) * mix(1.0 - shadow_ambient_factor, 1.0, shadow);

    vec3 color = ambient + Lo;
    out_color = vec4(color, 1.0);
}