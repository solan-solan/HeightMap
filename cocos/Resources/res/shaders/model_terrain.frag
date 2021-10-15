#ifdef GL_ES
    precision mediump float;

    #ifdef SHADOW
        varying mediump vec4 v_smcoord[DEPTH_TEXT_COUNT];
        varying mediump float v_shadow_fade_dist;
    #endif

    varying mediump vec2 v_texCoord[LAYER_TEXTURE_SIZE];
    #ifndef FIRST_LOD
        varying mediump float v_height;
        varying mediump vec2 v_dist_lod;
        varying mediump vec2 v_lod_radius;
    #endif
    #ifdef TEXT_LOD
        varying mediump vec2 v_texCoord_lod[LAYER_TEXTURE_SIZE];
        varying mediump float v_dist_alpha;
    #endif
    #ifdef MULTI_LAYER
        varying mediump float v_dist_alpha_layer;
        varying mediump float v_layer_alpha;
    #endif
    varying mediump float v_darker_dist;
    #ifdef FOG
        varying mediump float v_fogFactor;
    #endif
    varying mediump vec3 v_dirToCamera;
    
    varying mediump float v_alpha[LAYER_TEXTURE_SIZE];

    #ifdef NORMAL_MAP
        varying mediump vec3 v_DirLightSun;
    #else
        varying mediump vec3 v_normal;
    #endif
#else

    #ifdef SHADOW
        varying vec4 v_smcoord[DEPTH_TEXT_COUNT];
        varying float v_shadow_fade_dist;
    #endif

    varying vec2 v_texCoord[LAYER_TEXTURE_SIZE];
    #ifndef FIRST_LOD
        varying float v_height;
        varying vec2 v_dist_lod;
        varying vec2 v_lod_radius;
    #endif
    #ifdef TEXT_LOD
        varying vec2 v_texCoord_lod[LAYER_TEXTURE_SIZE];
        varying float v_dist_alpha;
    #endif
    #ifdef MULTI_LAYER
        varying float v_dist_alpha_layer;
        varying float v_layer_alpha;
    #endif
    varying float v_darker_dist;
    #ifdef FOG
        varying float v_fogFactor;
    #endif
    varying vec3 v_dirToCamera;

    varying float v_alpha[LAYER_TEXTURE_SIZE];

    #ifdef NORMAL_MAP
        varying vec3 v_DirLightSun;
    #else
        varying vec3 v_normal;
    #endif
#endif

    uniform sampler2D u_text[LAYER_TEXTURE_SIZE];

#ifdef NORMAL_MAP
    uniform sampler2D u_text_n[LAYER_TEXTURE_SIZE];
    uniform float u_normal_scale[LAYER_TEXTURE_SIZE];
#endif

#ifdef SHADOW
    uniform sampler2D u_text_sh[DEPTH_TEXT_COUNT];
    uniform vec2 u_texelSize_sh[DEPTH_TEXT_COUNT];
    uniform float u_smooth_rate_sh;
    uniform float u_light_thr_sh; 
#endif

#ifndef NORMAL_MAP
    uniform vec3 u_DirLightSun;
#endif
    uniform vec3 u_ColLightSun;
    uniform vec3 u_ColFog;
    uniform vec3 u_AmbientLightSourceColor;
    uniform float u_specular_factor[LAYER_TEXTURE_SIZE];

#ifdef SHADOW
    float shadowCalc()
    {
        float sh_cf[DEPTH_TEXT_COUNT + 1];
        sh_cf[0] = 1.0;
        int sh_cnt = 0;
        float bias = 0.0005; // Add bias to reduce shadow acne (error margin)
        for (int i = 0; i < DEPTH_TEXT_COUNT; ++i)
        {
            vec4 shMapPos = v_smcoord[i] / v_smcoord[i].w;

            if (all(bvec2(all(bvec4(lessThanEqual(shMapPos.xy, vec2(1.0, 1.0)), greaterThanEqual(shMapPos.xy, vec2(0.0, 0.0)))),
                all(bvec2(step(0.0, shMapPos.z), step(shMapPos.z, 1.0))))))
            {
                sh_cnt += 1;
                float shadow = 0.0;
                vec2 texelSize = 1.0 / u_texelSize_sh[i];

                for (float x = -u_smooth_rate_sh; x <= u_smooth_rate_sh; x += 1.0)
                {
                     for (float y = -u_smooth_rate_sh; y <= u_smooth_rate_sh; y += 1.0)
                    {
                        float distanceFromLight = texture2D(u_text_sh[i], shMapPos.st + vec2(x, y) * texelSize).z;

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
#ifndef FIRST_LOD
    float lod_alpha_x = (v_dist_lod.x - v_lod_radius.y) / (v_lod_radius.x - v_lod_radius.y);
    lod_alpha_x = clamp(lod_alpha_x, 0.0, 1.0);
    float lod_alpha_z = (v_dist_lod.y - v_lod_radius.y) / (v_lod_radius.x - v_lod_radius.y);
    lod_alpha_z = clamp(lod_alpha_z, 0.0, 1.0);
    float lod_alpha = length(vec2(lod_alpha_x, lod_alpha_z));
    lod_alpha = clamp(lod_alpha, 0.0, 1.0);

    if (lod_alpha < 0.2 && v_height >= -255.0)
        discard;
#endif

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
    for (int i = 0; i < LAYER_TEXTURE_SIZE; ++i)
    {
        max_v_alpha = max(max_v_alpha, v_alpha[i]);
        color += texture2D(u_text[i], v_texCoord[i]) * v_alpha[i];
        specular_factor += u_specular_factor[i] * v_alpha[i];

    #ifdef TEXT_LOD
        color_lod += texture2D(u_text[i], v_texCoord_lod[i]) * v_alpha[i];
    #endif

    #ifdef NORMAL_MAP
        normal += (2.0 * texture2D(u_text_n[i], v_texCoord[i]).xyz - 1.0) * v_alpha[i];
        normal_scale += u_normal_scale[i] * v_alpha[i];

        #ifdef TEXT_LOD
            normal_lod += (2.0 * texture2D(u_text_n[i], v_texCoord_lod[i]).xyz - 1.0) * v_alpha[i];
        #endif
    #endif
    }

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
    color.a *= lod_alpha;
#endif

    gl_FragColor = color;
}
