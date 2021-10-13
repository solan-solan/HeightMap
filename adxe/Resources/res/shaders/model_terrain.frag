#ifdef GL_ES
    precision mediump float;

    #ifdef SHADOW
        varying mediump vec4 v_smcoord;
    #endif

    varying mediump vec2 v_texCoord[LAYER_TEXTURE_SIZE];
    #ifndef FIRST_LOD
        varying mediump float v_lod_alpha;
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
        varying vec4 v_smcoord;
    #endif

    varying vec2 v_texCoord[LAYER_TEXTURE_SIZE];
    #ifndef FIRST_LOD
        varying float v_lod_alpha;
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
    uniform sampler2D u_text_sh;
    uniform vec2 u_texelSize_sh;
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
        
        vec4 shadowMapPosition_0 = v_smcoord / v_smcoord.w;

        if (all(bvec2(all(bvec4(lessThanEqual(shadowMapPosition_0.xy, vec2(1.0, 1.0)), greaterThanEqual(shadowMapPosition_0.xy, vec2(0.0, 0.0)))),
        all(bvec2(step(0.0, shadowMapPosition_0.z), step(shadowMapPosition_0.z, 1.0))))))
        {
            float shadow = 0.0;
            vec2 texelSize = 1.0 / u_texelSize_sh;
            // Add bias to reduce shadow acne (error margin)
            float bias = 0.0005;

            for (float x = -1.0; x <= 1.0; x += 1.0)
            {
                for (float y = -1.0; y <= 1.0; y += 1.0)
                {
                    float distanceFromLight = texture2D(u_text_sh, shadowMapPosition_0.st + vec2(x, y) * texelSize).z;

                    // 1.0 = not in shadow (fragment is closer to light than the value stored in shadow map)
                    // 0.0 = in shadow
                    shadow += float(distanceFromLight > shadowMapPosition_0.z - bias);
                }
            }

            shadow /= 9.0;
            return shadow;
        }
        
        return 1.0;
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
    shadow = (shadow * 0.8) + 0.2;
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

    gl_FragColor = color;
}
