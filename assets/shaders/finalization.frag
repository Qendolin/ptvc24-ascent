#version 450 core

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D u_color_tex;
layout(binding = 1) uniform sampler2D u_depth_tex;
layout(binding = 2) uniform sampler2D u_bloom_tex;
layout(binding = 3) uniform sampler2D u_flares_tex;
layout(binding = 4) uniform sampler2D u_glare_tex;
layout(binding = 5) uniform sampler2D u_ao_tex;

uniform float u_bloom_fac;
uniform float u_flares_fac;
uniform mat4 u_inverse_projection_mat;
uniform mat4 u_inverse_view_mat;
uniform vec3 u_camera_pos;

uniform float u_fog_density;
uniform float u_fog_emission;
uniform float u_fog_height;
uniform float u_fog_max;
uniform vec3 u_fog_color;


// dither matrix, use as dither_matrix[y][x] / 256.0
const float dither_matrix[16][16] = {
    {  0., 128.,  32., 160.,   8., 136.,  40., 168.,   2., 130.,  34., 162.,  10., 138.,  42., 170.},
    {192.,  64., 224.,  96., 200.,  72., 232., 104., 194.,  66., 226.,  98., 202.,  74., 234., 106.},
    { 48., 176.,  16., 144.,  56., 184.,  24., 152.,  50., 178.,  18., 146.,  58., 186.,  26., 154.},
    {240., 112., 208.,  80., 248., 120., 216.,  88., 242., 114., 210.,  82., 250., 122., 218.,  90.},
    { 12., 140.,  44., 172.,   4., 132.,  36., 164.,  14., 142.,  46., 174.,   6., 134.,  38., 166.},
    {204.,  76., 236., 108., 196.,  68., 228., 100., 206.,  78., 238., 110., 198.,  70., 230., 102.},
    { 60., 188.,  28., 156.,  52., 180.,  20., 148.,  62., 190.,  30., 158.,  54., 182.,  22., 150.},
    {252., 124., 220.,  92., 244., 116., 212.,  84., 254., 126., 222.,  94., 246., 118., 214.,  86.},
    {  3., 131.,  35., 163.,  11., 139.,  43., 171.,   1., 129.,  33., 161.,   9., 137.,  41., 169.},
    {195.,  67., 227.,  99., 203.,  75., 235., 107., 193.,  65., 225.,  97., 201.,  73., 233., 105.},
    { 51., 179.,  19., 147.,  59., 187.,  27., 155.,  49., 177.,  17., 145.,  57., 185.,  25., 153.},
    {243., 115., 211.,  83., 251., 123., 219.,  91., 241., 113., 209.,  81., 249., 121., 217.,  89.},
    { 15., 143.,  47., 175.,   7., 135.,  39., 167.,  13., 141.,  45., 173.,   5., 133.,  37., 165.},
    {207.,  79., 239., 111., 199.,  71., 231., 103., 205.,  77., 237., 109., 197.,  69., 229., 101.},
    { 63., 191.,  31., 159.,  55., 183.,  23., 151.,  61., 189.,  29., 157.,  53., 181.,  21., 149.},
    {255., 127., 223.,  95., 247., 119., 215.,  87., 253., 125., 221.,  93., 245., 117., 213.,  85.}
};

// Mean error^2: 3.6705141e-06
vec3 agxDefaultContrastApprox(vec3 x) {
    vec3 x2 = x * x;
    vec3 x4 = x2 * x2;
  
    return + 15.5     * x4 * x2
           - 40.14    * x4 * x
           + 31.96    * x4
           - 6.868    * x2 * x
           + 0.4298   * x2
           + 0.1191   * x
           - 0.00232;
}

vec3 agx(vec3 val) {
    const mat3 agx_mat = mat3(
        0.842479062253094, 0.0423282422610123, 0.0423756549057051,
        0.0784335999999992,  0.878468636469772,  0.0784336,
        0.0792237451477643, 0.0791661274605434, 0.879142973793104);

    const float min_ev = -12.47393;
    const float max_ev = 4.026069;
    const float mid_gray = 1.0;

    // Input transform
    val = agx_mat * val;

    // Log2 space encoding
    val = clamp(log2(val / mid_gray), min_ev, max_ev);
    val = (val - min_ev) / (max_ev - min_ev);

    // Apply sigmoid function approximation
    val = agxDefaultContrastApprox(val);

    return val;
}

vec3 agxEotf(vec3 val) {
    const mat3 agx_mat_inv = mat3(
        1.19687900512017, -0.0528968517574562, -0.0529716355144438,
        -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
        -0.0990297440797205, -0.0989611768448433, 1.15107367264116);

    // Undo input transform
    val = agx_mat_inv * val;

    return val;
}

vec3 agxLook(vec3 val) {
    const vec3 lw = vec3(0.2126, 0.7152, 0.0722);
    float luma = dot(val, lw);

    // Default
    vec3 offset = vec3(0.0);
    vec3 slope = vec3(1.0);
    vec3 power = vec3(1.0);
    float sat = 1.0;

    // Punchy
    offset = vec3(0.1);
    slope = vec3(0.95);
    power = vec3(1.2, 1.2, 1.2);
    sat = 1.4;

    // ASC CDL
    val = pow(val * slope + offset, power);
    return luma + sat * (val - luma);
}


// Reference: https://www.shadertoy.com/view/cd3XWr
vec3 tonemapAgX(vec3 col) {
    col = agx(col);
    col = agxLook(col);
    col = agxEotf(col);
    return col;
}

vec3 dither(vec3 col) {
    int x = int(gl_FragCoord.x) % 8;
    int y = int(gl_FragCoord.y) % 8;
    const float matrix_normalization = 1. / 256.;
    const float rgb_normalization = 1. / 255.;
    vec3 value = vec3((dither_matrix[y][x] - 127.) * matrix_normalization);
    return col + value * rgb_normalization;
}

// Inigo Quilez's fog
vec3 applyFog(vec3 frag_color, float d, float d_xz, vec3 ray_origin, vec3 ray_direction) {
    if(ray_direction.y == 0.0) ray_direction.y = 0.000001;

    if(d > 100000.0) { // skybox
        // Looks kinda bad when inside the height based fog
    	float fog = clamp(ray_direction.y + 0.1, 0.0, 1.0);
        fog = pow(1.0-fog, 7.0);
        return mix(frag_color, u_fog_color, fog);
    }

    // height based fog
    // FIXME: breaks whe the camera is too high because the exponentail values get too extreme
    float fog_amount = (u_fog_emission/u_fog_density) * exp(-(ray_origin.y-u_fog_height) * u_fog_density) * (1.0 - exp(-d * ray_direction.y * u_fog_density)) / ray_direction.y;

    fog_amount = clamp(fog_amount, 0.0, u_fog_max);

    // world border fog
    fog_amount += pow(max(d_xz - 5000.0, 0.0)*0.0001, 2.0);

    // const float fog_bloom_brightness_correction = 1.52; // simply estimated
    return mix(frag_color, u_fog_color, min(fog_amount, 1.0));
}

vec3 reconstruct_view_space_position(float depth, vec2 uv) {
    vec2 clip_xy = uv * 2.0 - 1.0;
    vec4 t = u_inverse_projection_mat * vec4(clip_xy, depth, 1.0);
    return t.xyz / t.w;
}


vec3 load_and_reconstruct_view_space_position(vec2 uv) {
    float depth = texture(u_depth_tex, uv).r;
    depth = max(depth, 0.000001); // prevent infinity issues 
    return reconstruct_view_space_position(depth, uv);
}

void main() {
    vec3 color = texture(u_color_tex, in_uv).rgb;

    // GTAO (TODO: apply in pbr shader)
    color *= clamp(texture(u_ao_tex, in_uv).r, 0.0, 1.0);
    // FIXME: ao looks bad on terrain

    // Bloom
    color += texture(u_bloom_tex, in_uv).rgb * u_bloom_fac;

    // Flares & Glare
    color += texture(u_flares_tex, in_uv).rgb * u_flares_fac;
    color += texture(u_glare_tex, in_uv).rgb * u_flares_fac;
    
    // Fog
    vec3 view_position = load_and_reconstruct_view_space_position(in_uv);
    vec3 world_position = (u_inverse_view_mat * vec4(view_position, 1.0)).xyz;
    float view_distance = length(world_position - u_camera_pos);
    float view_distance_xz = length(world_position.xz - u_camera_pos.xz);
    color = applyFog(color, view_distance, view_distance_xz, u_camera_pos, (world_position - u_camera_pos) / view_distance);

    // Tonemapping
    color = tonemapAgX(color);

    // dithering
    color = dither(color);

    out_color = vec4(color, 1.);

    // Not really needed
    // gl_FragDepth = texture(u_depth_tex, in_uv).x;
}