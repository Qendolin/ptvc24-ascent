#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D u_albedo_tex;
layout(binding = 1) uniform sampler2D u_metallic_roughness_tex;
layout(binding = 2) uniform sampler2D u_normal_tex;

uniform vec3 u_camera_pos;
uniform vec3 u_albedo_fac;
uniform vec2 u_metallic_roughness_fac;

const float PI = 3.14159265359;

float distribution_ggx(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    // when roughness is zero and N = H denom would be 0
    denom = PI * denom * denom + 5e-6;

    return nom / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness)
{
    // + 5e-6 to prevent artifacts, value is from https://google.github.io/filament/Filament.html#materialsystem/specularbrdf:~:text=float%20NoV%20%3D%20abs(dot(n%2C%20v))%20%2B%201e%2D5%3B
    float NdotV = max(dot(N, V), 0.0) + 5e-6;
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 albedo             = texture(u_albedo_tex, in_uv).rgb             * u_albedo_fac;
    vec2 metallic_roughness = texture(u_metallic_roughness_tex, in_uv).bg  * u_metallic_roughness_fac;
    float metallic  = metallic_roughness.x;
    float roughness = metallic_roughness.y;

    vec3 N = normalize(in_normal);
    vec3 P = in_position;
    vec3 V = normalize(u_camera_pos - P);
    vec3 R = reflect(-V, N);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    // for(int i = 0; i < ORTHO_LIGHT_COUNT + POINT_LIGHT_COUNT; ++i)
    for(int i = 0; i < 1; ++i)
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
        vec3 H = normalize(V + L);

        // Cook-Torrance BRDF
        float NDF = distribution_ggx(N, H, roughness);
        float G   = geometry_smith(N, V, L, roughness);
        vec3 F    = fresnel_schlick(max(dot(H, V), 0.0), F0);

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

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // ambient lighting
    vec3 kS = fresnel_schlick(max(dot(N, V), 0.0), F0);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    vec3 irradiance = vec3(0.8, 1.0, 1.0) * 0.03;
    vec3 diffuse = irradiance * albedo;
    vec3 ambient = (kD * diffuse);

    vec3 color = ambient + Lo;
    
    // gamma correction
    out_color = vec4(pow(color, vec3(1.0/2.2)), 1.0);
}