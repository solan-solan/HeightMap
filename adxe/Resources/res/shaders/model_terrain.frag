
precision highp float;

out vec4 FragColor;

//-- Definitions
#define vfloat_def(x, y) vec4 x[(y + 3) / 4]
#define vfloat_at(x, y) x[y / 4][y % 4]

#define vvec2_def(x, y) vec4 x[(y * 2 + 3) / 4]
#define vvec2_at1(x, y, z) x[(y / 2)][y % 2 * 2 + z]
#define vvec2_at(x, y) vec2(vvec2_at1(x, y, 0), vvec2_at1(x, y, 1))

#define COLOR(NUM, max_v_alpha, color, specular_factor) \
    max_v_alpha = max(max_v_alpha, v_alpha[NUM]); \
    color += texture(u_text[NUM], v_texCoord[NUM]) * v_alpha[NUM]; \
    specular_factor += vfloat_at(u_specular_factor, NUM) * v_alpha[NUM]

#define COLOR_LOD(NUM, color_lod) \
    color_lod += texture(u_text[NUM], v_texCoord_lod[NUM]) * v_alpha[NUM]

#define NORMAL(NUM, normal, normal_scale) \
    normal += (2.0 * texture(u_text_n[NUM], v_texCoord[NUM]).xyz - 1.0) * v_alpha[NUM]; \
    normal_scale += vfloat_at(u_normal_scale, NUM) * v_alpha[NUM]

#define NORMAL_COLOR_LOD(NUM, normal, normal_scale, normal_lod) \
    normal += (2.0 * texture(u_text_n[NUM], v_texCoord[NUM]).xyz - 1.0) * v_alpha[NUM]; \
    normal_scale += vfloat_at(u_normal_scale, NUM) * v_alpha[NUM]; \
    normal_lod += (2.0 * texture(u_text_n[NUM], v_texCoord_lod[NUM]).xyz - 1.0) * v_alpha[NUM]
//--

#ifdef SHADOW
    in vec4 v_smcoord[DEPTH_TEXT_COUNT];
    in float v_shadow_fade_dist;
#endif

in vec2 v_texCoord[LAYER_TEXTURE_SIZE];
#ifndef FIRST_LOD
    in float v_lod_alpha;
#endif
#ifdef TEXT_LOD
    in vec2 v_texCoord_lod[LAYER_TEXTURE_SIZE];
    in float v_dist_alpha;
#endif
#ifdef MULTI_LAYER
    in float v_dist_alpha_layer;
    in float v_layer_alpha;
#endif
in float v_darker_dist;
#ifdef FOG
    in float v_fogFactor;
#endif
in vec3 v_dirToCamera;

in float v_alpha[LAYER_TEXTURE_SIZE];

#ifdef NORMAL_MAP
    in vec3 v_DirLightSun;
#else
    in vec3 v_normal;
#endif

uniform sampler2D u_text[LAYER_TEXTURE_SIZE];
#ifdef NORMAL_MAP
    uniform sampler2D u_text_n[LAYER_TEXTURE_SIZE];
#endif
#ifdef SHADOW
    uniform sampler2D u_text_sh[DEPTH_TEXT_COUNT]; 
#endif

layout(std140) uniform fs_ub {
    #ifdef NORMAL_MAP
        vfloat_def(u_normal_scale, LAYER_TEXTURE_SIZE);
    #endif

    #ifdef SHADOW
        vvec2_def(u_texelSize_sh, DEPTH_TEXT_COUNT);
        uniform float u_smooth_rate_sh;
        uniform float u_light_thr_sh; 
    #endif

    #ifndef NORMAL_MAP
        uniform vec3 u_DirLightSun;
    #endif
    uniform vec3 u_ColLightSun;
    uniform vec3 u_ColFog;
    uniform vec3 u_AmbientLightSourceColor;
    vfloat_def(u_specular_factor, LAYER_TEXTURE_SIZE);
};


#ifdef SHADOW
    vec4 v_smcoord_literal(in int i)
    {
        vec4 val;
        switch(i)
        {
            case 0:
                val = v_smcoord[0] / v_smcoord[0].w;
                break;
        #if (DEPTH_TEXT_COUNT > 1)
            case 1:
                val = v_smcoord[1] / v_smcoord[1].w;
                break;
        #endif
            default:
                val = vec4(0.0, 0.0, 0.0, 1.0);
        }
        return val;
    }

    vec4 texture_sh_by_index(in vec2 uv, in int idx)
    {
        vec4 val;
        switch(idx)
        {
            case 0:
                val = texture(u_text_sh[0], uv);
                break;
        #if (DEPTH_TEXT_COUNT > 1)
            case 1:
                val = texture(u_text_sh[1], uv);
                break;
        #endif
            default:
                val = vec4(0.0, 0.0, 0.0, 1.0);
        }
        return val;
    }

    float shadowCalc()
    {
        float sh_cf[DEPTH_TEXT_COUNT + 1];
        sh_cf[0] = 1.0;
        int sh_cnt = 0;
        float bias = 0.0005; // Add bias to reduce shadow acne (error margin)
        for (int i = 0; i < DEPTH_TEXT_COUNT; ++i)
        {
            vec4 shMapPos = v_smcoord_literal(i);

            if (all(bvec2(all(bvec4(lessThanEqual(shMapPos.xy, vec2(1.0, 1.0)), greaterThanEqual(shMapPos.xy, vec2(0.0, 0.0)))),
                all(bvec2(step(0.0, shMapPos.z), step(shMapPos.z, 1.0))))))
            {
                sh_cnt += 1;
                float shadow = 0.0;
                vec2 texelSize = 1.0 / vvec2_at(u_texelSize_sh, i);

                for (float x = -u_smooth_rate_sh; x <= u_smooth_rate_sh; x += 1.0)
                {
                    for (float y = -u_smooth_rate_sh; y <= u_smooth_rate_sh; y += 1.0)
                    {
                        float distanceFromLight = texture_sh_by_index(shMapPos.st + vec2(x, y) * texelSize, i).r;
                        // 1.0 = not in shadow (fragment is closer to light than the value stored in shadow map)
                        // 0.0 = in shadow
                        shadow += step(shMapPos.z - bias, distanceFromLight);
                    }
                }

                shadow /= (u_smooth_rate_sh * 2.0 + 1.0) * (u_smooth_rate_sh * 2.0 + 1.0);
                sh_cf[sh_cnt] = min(shadow, sh_cf[sh_cnt - 1]);
            }
        }
        return sh_cf[sh_cnt];
    }
#endif

vec3 computeLighting(vec3 normalVector, vec3 lightDirection, vec3 dirToCamera, float specular_factor)
{
    float attenuation = 1.0;

    float diffuse = max(dot(normalVector, -lightDirection), 0.0);
    diffuse = pow(diffuse, 2.0);
    vec3 diffuseColor = u_ColLightSun  * diffuse * attenuation;

    vec3 lightReflect = normalize(reflect(lightDirection, normalVector));
    float specular_coeff = pow(max(0.0, dot(dirToCamera, lightReflect)), 16.0);

    diffuseColor += u_ColLightSun * specular_coeff * specular_factor;

    return diffuseColor;
}

void main()
{
    vec4 combinedColor = vec4(u_AmbientLightSourceColor, 1.0);
    
    vec4 color;
    float specular_factor = 0.0;
    #ifdef TEXT_LOD
        vec4 color_lod;
    #endif
    #ifdef NORMAL_MAP
        vec3 normal;
        float normal_scale = 0.0;
        #ifdef TEXT_LOD
            vec3 normal_lod;
        #endif
    #endif
    float max_v_alpha = 0.0;

    // Process texture data
    COLOR(0, max_v_alpha, color, specular_factor);
    #ifdef TEXT_LOD
        COLOR_LOD(0, color_lod);
    #endif
    #ifdef NORMAL_MAP
        #ifdef TEXT_LOD
            NORMAL_COLOR_LOD(0, normal, normal_scale, normal_lod);
        #else
            NORMAL(0, normal, normal_scale);
        #endif
    #endif

    #if (LAYER_TEXTURE_SIZE > 1)
        COLOR(1, max_v_alpha, color, specular_factor);
        #ifdef TEXT_LOD
            COLOR_LOD(1, color_lod);
        #endif
        #ifdef NORMAL_MAP
            #ifdef TEXT_LOD
                NORMAL_COLOR_LOD(1, normal, normal_scale, normal_lod);
            #else
                NORMAL(1, normal, normal_scale);
            #endif
        #endif
    #endif

    #if (LAYER_TEXTURE_SIZE > 2)
        COLOR(2, max_v_alpha, color, specular_factor);
        #ifdef TEXT_LOD
            COLOR_LOD(2, color_lod);
        #endif
        #ifdef NORMAL_MAP
            #ifdef TEXT_LOD
                NORMAL_COLOR_LOD(2, normal, normal_scale, normal_lod);
            #else
                NORMAL(2, normal, normal_scale);
            #endif
        #endif
    #endif

    #if (LAYER_TEXTURE_SIZE > 3)
        COLOR(3, max_v_alpha, color, specular_factor);
        #ifdef TEXT_LOD
            COLOR_LOD(3, color_lod);
        #endif
        #ifdef NORMAL_MAP
            #ifdef TEXT_LOD
                NORMAL_COLOR_LOD(3, normal, normal_scale, normal_lod);
            #else
                NORMAL(3, normal, normal_scale);
            #endif
        #endif
    #endif
    // End Process texture data

/*    for (int i = 0; i < LAYER_TEXTURE_SIZE; ++i)
    {
        max_v_alpha = max(max_v_alpha, v_alpha[i]);
        color += texture(u_text[i], v_texCoord[i]) * v_alpha[i];
        specular_factor += u_specular_factor[i] * v_alpha[i];

    #ifdef TEXT_LOD
        color_lod += texture(u_text[i], v_texCoord_lod[i]) * v_alpha[i];
    #endif

    #ifdef NORMAL_MAP
        normal += (2.0 * texture(u_text_n[i], v_texCoord[i]).xyz - 1.0) * v_alpha[i];
        normal_scale += u_normal_scale[i] * v_alpha[i];

        #ifdef TEXT_LOD
            normal_lod += (2.0 * texture(u_text_n[i], v_texCoord_lod[i]).xyz - 1.0) * v_alpha[i];
        #endif
    #endif
    }*/

    #ifdef TEXT_LOD
        color = mix(color_lod, color, v_dist_alpha);
    #endif

    #ifdef NORMAL_MAP
        #ifdef TEXT_LOD
            normal = mix(normal_lod, normal, v_dist_alpha);
        #endif

        normal = normalize(normal);
        normal.xy *= normal_scale;
        normal = normalize(normal);

        combinedColor.xyz += computeLighting(normal, v_DirLightSun, v_dirToCamera, specular_factor);
    #else    
        combinedColor.xyz += computeLighting(v_normal, u_DirLightSun, v_dirToCamera, specular_factor);
    #endif

    color = color * combinedColor * v_darker_dist;

#ifdef SHADOW
    // If the fragment is not behind light view frustum
    float shadow = shadowCalc();
    // Scale 0.0-1.0 to 0.2-1.0
    // otherways everything in shadow would be black
    shadow = (shadow * (1.0 - u_light_thr_sh)) + u_light_thr_sh;
    shadow = mix(shadow, 1.0, v_shadow_fade_dist);
    color *= shadow;
#endif

#ifdef MULTI_LAYER
    color.a = max(max_v_alpha, v_layer_alpha);
#else
    color.a = 1.0;
#endif

#ifdef FOG
    color = mix(vec4(u_ColFog, 1.0), color, v_fogFactor);
#endif

#ifdef MULTI_LAYER
    color.a *= v_dist_alpha_layer;
#endif

#ifndef FIRST_LOD
    color.a *= v_lod_alpha;
#endif

    FragColor = color;
}
