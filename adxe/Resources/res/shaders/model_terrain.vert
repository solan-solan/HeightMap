// Definitions
#define vvec2_def(x, y) vec4 x[(y * 2 + 3) / 4]
#define vvec2_at1(x, y, z) x[(y / 2)][y % 2 * 2 + z]
#define vvec2_at(x, y) vec2(vvec2_at1(x, y, 0), vvec2_at1(x, y, 1))
//


in float a_height;
in float a_npack;
in float a_vertex_x;
in float a_vertex_z;
#if MAX_LAYER_COUNT == 4
    in vec4 a_alpha;
#elif MAX_LAYER_COUNT == 3
    in vec3 a_alpha;
#elif MAX_LAYER_COUNT == 2
    in vec2 a_alpha;
#elif MAX_LAYER_COUNT == 1
    in float a_alpha;
#endif

layout(std140) uniform vs_ub {
    #ifdef SHADOW
        uniform mat4 u_lVP_sh[DEPTH_TEXT_COUNT];
        uniform float u_shadow_fade_dist;
    #endif

    uniform vec3 u_camPos;
    uniform mat4 u_VPMatrix;
    #ifndef FIRST_LOD
        uniform vec4 u_Ndraw;
        uniform vec3 u_lod_radius;
    #endif
    uniform float u_layer_num;
    vvec2_def(u_text_tile_size, LAYER_TEXTURE_SIZE);
    uniform vec3 u_scale;
    uniform float u_darker_dist;
    #ifdef TEXT_LOD
        uniform vec2 u_text_lod_distance;
    #endif
    #ifdef MULTI_LAYER
        uniform vec2 u_dist_alpha_layer;
    #endif

    #ifdef FOG
        uniform float u_near_fog_plane;
        uniform float u_far_fog_plane;
    #endif

    #ifdef NORMAL_MAP
        uniform vec3 u_DirLightSun;
    #endif
};

#ifdef SHADOW
    out vec4 v_smcoord[DEPTH_TEXT_COUNT];
    out float v_shadow_fade_dist;
#endif
out vec2 v_texCoord[LAYER_TEXTURE_SIZE];
#ifndef FIRST_LOD
    out float v_lod_alpha;
#endif
#ifdef TEXT_LOD
    out vec2 v_texCoord_lod[LAYER_TEXTURE_SIZE];
    out float v_dist_alpha;
#endif
#ifdef MULTI_LAYER
    out float v_dist_alpha_layer;
    out float v_layer_alpha;
#endif
out float v_darker_dist;
#ifdef FOG
    out float v_fogFactor;
#endif
out vec3 v_dirToCamera;
out vec3 v_normal;
out float v_alpha[LAYER_TEXTURE_SIZE];

#ifdef NORMAL_MAP
    out vec3 v_DirLightSun;
#endif

void main()
{
    float x = a_vertex_x;
    float z = a_vertex_z;

#ifndef FIRST_LOD
    vec2 xz_ndraw = vec2(x, z);
    float dist_x = distance(x, u_camPos.x);
    float dist_z = distance(z, u_camPos.z);
    float lod_alpha_x = 1.0;
    float lod_alpha_z = 1.0;
    
    lod_alpha_x = (dist_x - u_lod_radius.z) / (u_lod_radius.x - u_lod_radius.z);
    lod_alpha_x = clamp(lod_alpha_x, 0.0, 1.0);
    lod_alpha_z = (dist_z - u_lod_radius.z) / (u_lod_radius.x - u_lod_radius.z);
    lod_alpha_z = clamp(lod_alpha_z, 0.0, 1.0);
    v_lod_alpha = length(vec2(lod_alpha_x, lod_alpha_z));
    v_lod_alpha = clamp(v_lod_alpha, 0.0, 1.0);

    float h = a_height * u_scale.y - 100000.0 * float(all(bvec4(lessThan(xz_ndraw.xy, u_Ndraw.yw), greaterThan(xz_ndraw.xy, u_Ndraw.xz))));
#else
    float h = a_height * u_scale.y;
#endif

	vec3 pos = vec3(x, h, z);

    v_dirToCamera = normalize(u_camPos - pos);

    for (int i = 0; i < LAYER_TEXTURE_SIZE; ++i)
    {
        float text_tile_size = vvec2_at(u_text_tile_size, i).x;//u_text_tile_size[i].x;
        v_texCoord[i] = vec2((x + text_tile_size) / (2.0 * text_tile_size), 
            (z + text_tile_size) / (2.0 * text_tile_size));

        #ifdef TEXT_LOD
            text_tile_size = vvec2_at(u_text_tile_size, i).y;
            v_texCoord_lod[i] = vec2((x + text_tile_size) / (2.0 * text_tile_size), 
                (z + text_tile_size) / (2.0 * text_tile_size));
        #endif
    }

#if MAX_LAYER_COUNT == 1
    float alpha = a_alpha;
#else        
    float alpha = a_alpha[int(u_layer_num)];
#endif

    v_alpha[0] = mod(alpha, 256.0) / 255.0;
#if LAYER_TEXTURE_SIZE > 1
    v_alpha[1] = alpha / 256.0;
    v_alpha[1] = mod(v_alpha[1], 256.0) / 255.0;
#endif
#if LAYER_TEXTURE_SIZE > 2
    v_alpha[2] = alpha / 65536.0;
    v_alpha[2] = mod(v_alpha[2], 256.0) / 255.0;
#endif
#if LAYER_TEXTURE_SIZE == 4
    v_alpha[3] = (alpha / 16777216.0) / 255.0;
#endif

    float xn = a_npack / 65536.0 - 128.0;
    float yn = a_npack / 256.0;
    yn = mod(yn, 256.0) - 128.0;
    float zn = mod(a_npack, 256.0) - 128.0;
    v_normal = vec3(xn / 127.0, yn / 127.0, zn / 127.0);

#ifdef NORMAL_MAP
    vec3 tangent = normalize(cross(v_normal, vec3(0.0, 0.0, 1.0)));
    vec3 bitangent = normalize(cross(v_normal, tangent));

    v_DirLightSun = vec3(dot(tangent, u_DirLightSun), dot(bitangent, u_DirLightSun), dot(v_normal, u_DirLightSun));
    v_dirToCamera = vec3(dot(tangent, v_dirToCamera), dot(bitangent, v_dirToCamera), dot(v_normal, v_dirToCamera));
#endif

    vec4 pp = u_VPMatrix * vec4(pos, 1.0);

    v_darker_dist = 1.0 - abs(pp.z) / u_darker_dist;
    v_darker_dist = clamp(v_darker_dist, 0.4, 1.0);

#ifdef TEXT_LOD
    v_dist_alpha = 1.0 - (abs(pp.z) - (u_text_lod_distance.y)) / (u_text_lod_distance.x);
    v_dist_alpha = clamp(v_dist_alpha, 0.0, 1.0);
#endif

#ifdef MULTI_LAYER
    v_dist_alpha_layer = 1.0 - ((abs(pp.z) - (u_dist_alpha_layer.y)) / (u_dist_alpha_layer.x)) * step(0.15, u_layer_num);
    v_dist_alpha_layer = clamp(v_dist_alpha_layer, 0.0, 1.0);
    v_dist_alpha_layer = -1.0 * v_dist_alpha_layer * (v_dist_alpha_layer - 2.0);

    v_layer_alpha = step(u_layer_num, 0.15);
#endif

#ifndef FIRST_LOD
    pp.z -= u_lod_radius.y;
#endif

#ifdef SHADOW
    for (int i = 0; i < DEPTH_TEXT_COUNT; ++i)
        v_smcoord[i] = u_lVP_sh[i] * vec4(pos, 1.0);

    v_shadow_fade_dist = abs(pp.z) / u_shadow_fade_dist;
    v_shadow_fade_dist = clamp(v_shadow_fade_dist, 0.0, 1.0);
#endif

	gl_Position = pp;

#ifdef FOG
    float fogFragCoord = abs(pp.z);
    v_fogFactor = (u_far_fog_plane - fogFragCoord) / (u_far_fog_plane - u_near_fog_plane);
    v_fogFactor = clamp(v_fogFactor, 0.0, 1.0);
#endif
}
