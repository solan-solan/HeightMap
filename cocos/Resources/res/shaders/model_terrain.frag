#ifdef GL_ES
    precision mediump float;

    varying mediump vec2 v_texCoord[LAYER_TEXTURE_SIZE];
    #ifdef TEXT_LOD
        varying mediump vec2 v_texCoord_lod[LAYER_TEXTURE_SIZE];
        varying mediump float v_dist_alpha;
    #endif
    #ifdef MULTI_LAYER
        varying mediump float v_dist_alpha_layer;
    #endif
    varying mediump float v_darker_dist;
    #ifdef FOG
        varying mediump float v_fogFactor;
    #endif
    varying mediump vec3 v_dirToCamera;
    
    varying mediump float v_alpha[LAYER_TEXTURE_SIZE];
    varying mediump vec2 v_xz_ndraw;

    #ifdef NORMAL_MAP
        varying mediump vec3 v_DirLightSun;
    #else
        varying mediump vec3 v_normal;
    #endif
#else

    varying vec2 v_texCoord[LAYER_TEXTURE_SIZE];
    #ifdef TEXT_LOD
        varying vec2 v_texCoord_lod[LAYER_TEXTURE_SIZE];
        varying float v_dist_alpha;
    #endif
    #ifdef MULTI_LAYER
        varying float v_dist_alpha_layer;
    #endif
    varying float v_darker_dist;
    #ifdef FOG
        varying float v_fogFactor;
    #endif
    varying vec3 v_dirToCamera;

    varying float v_alpha[LAYER_TEXTURE_SIZE];
    varying vec2 v_xz_ndraw;

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

#ifndef NORMAL_MAP
    uniform vec3 u_DirLightSun;
#endif
    uniform vec3 u_ColLightSun;
    uniform vec3 u_ColFog;
    uniform vec3 u_AmbientLightSourceColor;
    uniform float u_specular_factor[LAYER_TEXTURE_SIZE];
    uniform vec4 u_Ndraw;

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

    if (all(bvec4(lessThan(v_xz_ndraw.xy, u_Ndraw.yw), greaterThan(v_xz_ndraw.xy, u_Ndraw.xz))) == true)
        discard;
    
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
    color.a = max_v_alpha;

#ifdef FOG
    color = mix(vec4(u_ColFog, 1.0), color, v_fogFactor);
#endif

#ifdef MULTI_LAYER
    color.a *= v_dist_alpha_layer;
#endif

   gl_FragColor = color;
}
