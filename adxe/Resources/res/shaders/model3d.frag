
precision highp float;

out vec4 FragColor;

layout(std140) uniform fs_ub {
    #if (MAX_DIRECTIONAL_LIGHT_NUM > 0)
        uniform vec3 u_DirLightSourceColor[MAX_DIRECTIONAL_LIGHT_NUM];
        uniform vec3 u_DirLightSourceDirection[MAX_DIRECTIONAL_LIGHT_NUM];
    #endif
    #if (MAX_POINT_LIGHT_NUM > 0)
        uniform vec3 u_PointLightSourceColor[MAX_POINT_LIGHT_NUM];
        uniform float u_PointLightSourceRangeInverse[MAX_POINT_LIGHT_NUM];
    #endif
    #if (MAX_SPOT_LIGHT_NUM > 0)
        uniform vec3 u_SpotLightSourceColor[MAX_SPOT_LIGHT_NUM];
        uniform vec3 u_SpotLightSourceDirection[MAX_SPOT_LIGHT_NUM];
        uniform float u_SpotLightSourceInnerAngleCos[MAX_SPOT_LIGHT_NUM];
        uniform float u_SpotLightSourceOuterAngleCos[MAX_SPOT_LIGHT_NUM];
        uniform float u_SpotLightSourceRangeInverse[MAX_SPOT_LIGHT_NUM];
    #endif
    uniform vec3 u_AmbientLightSourceColor;

    uniform vec4 u_color;
};

#ifdef USE_NORMAL_MAPPING
    uniform sampler2D u_normalTex;
#endif

uniform sampler2D u_tex0;
//uniform sampler2D u_tex_f;


in vec2 TextureCoordOut;

#ifdef USE_NORMAL_MAPPING
    #if MAX_DIRECTIONAL_LIGHT_NUM
        in vec3 v_dirLightDirection[MAX_DIRECTIONAL_LIGHT_NUM];
    #endif
#endif
#if MAX_POINT_LIGHT_NUM
    in vec3 v_vertexToPointLightDirection[MAX_POINT_LIGHT_NUM];
#endif
#if MAX_SPOT_LIGHT_NUM
    in vec3 v_vertexToSpotLightDirection[MAX_SPOT_LIGHT_NUM];
    #ifdef USE_NORMAL_MAPPING
        in vec3 v_spotLightDirection[MAX_SPOT_LIGHT_NUM];
    #endif
#endif

#ifndef USE_NORMAL_MAPPING
    #if ((MAX_DIRECTIONAL_LIGHT_NUM > 0) || (MAX_POINT_LIGHT_NUM > 0) || (MAX_SPOT_LIGHT_NUM > 0))
        in vec3 v_normal;
    #endif
#endif


vec3 computeLighting(vec3 normalVector, vec3 lightDirection, vec3 lightColor, float attenuation)
{
    float diffuse = max(dot(normalVector, lightDirection), 0.0);
    vec3 diffuseColor = lightColor  * diffuse * attenuation;

    return diffuseColor;
}

void main(void)
{

#ifdef USE_NORMAL_MAPPING
    #if ((MAX_DIRECTIONAL_LIGHT_NUM > 0) || (MAX_POINT_LIGHT_NUM > 0) || (MAX_SPOT_LIGHT_NUM > 0))
        vec3 normal  = normalize(2.0 * texture(u_normalTex, TextureCoordOut).xyz - 1.0);
    #endif
#else
    #if ((MAX_DIRECTIONAL_LIGHT_NUM > 0) || (MAX_POINT_LIGHT_NUM > 0) || (MAX_SPOT_LIGHT_NUM > 0))
        vec3 normal  = normalize(v_normal);
    #endif
#endif

    vec4 combinedColor = vec4(u_AmbientLightSourceColor, 1.0);

    // Directional light contribution
#if (MAX_DIRECTIONAL_LIGHT_NUM > 0)
    for (int i = 0; i < MAX_DIRECTIONAL_LIGHT_NUM; ++i)
    {
#ifdef USE_NORMAL_MAPPING
        vec3 lightDirection = normalize(v_dirLightDirection[i] * 2.0);
#else
        vec3 lightDirection = normalize(u_DirLightSourceDirection[i] * 2.0);
#endif
        combinedColor.xyz += computeLighting(normal, -lightDirection, u_DirLightSourceColor[i], 1.0);
    }
#endif

    // Point light contribution
#if (MAX_POINT_LIGHT_NUM > 0)
    for (int i = 0; i < MAX_POINT_LIGHT_NUM; ++i)
    {
        vec3 ldir = v_vertexToPointLightDirection[i] * u_PointLightSourceRangeInverse[i];
        float attenuation = clamp(1.0 - dot(ldir, ldir), 0.0, 1.0);
        combinedColor.xyz += computeLighting(normal, normalize(v_vertexToPointLightDirection[i]), u_PointLightSourceColor[i], attenuation);
    }
#endif

    // Spot light contribution
#if (MAX_SPOT_LIGHT_NUM > 0)
    for (int i = 0; i < MAX_SPOT_LIGHT_NUM; ++i)
    {
        // Compute range attenuation
        vec3 ldir = v_vertexToSpotLightDirection[i] * u_SpotLightSourceRangeInverse[i];
        float attenuation = clamp(1.0 - dot(ldir, ldir), 0.0, 1.0);
        vec3 vertexToSpotLightDirection = normalize(v_vertexToSpotLightDirection[i]);

#ifdef USE_NORMAL_MAPPING
        vec3 spotLightDirection = normalize(v_spotLightDirection[i] * 2.0);
#else
        vec3 spotLightDirection = normalize(u_SpotLightSourceDirection[i] * 2.0);
#endif

        // "-lightDirection" is used because light direction points in opposite direction to spot direction.
        float spotCurrentAngleCos = dot(spotLightDirection, -vertexToSpotLightDirection);

        // Apply spot attenuation
        attenuation *= smoothstep(u_SpotLightSourceOuterAngleCos[i], u_SpotLightSourceInnerAngleCos[i], spotCurrentAngleCos);
        attenuation = clamp(attenuation, 0.0, 1.0);
        combinedColor.xyz += computeLighting(normal, vertexToSpotLightDirection, u_SpotLightSourceColor[i], attenuation);
    }
#endif

#if ((MAX_DIRECTIONAL_LIGHT_NUM > 0) || (MAX_POINT_LIGHT_NUM > 0) || (MAX_SPOT_LIGHT_NUM > 0))

//    vec4 col_val = texture(u_tex_f, TextureCoordOut);
//    if (col_val.x == 2.56322)
//        col_val = vec4(0.0, 0.0, 0.0, 1.0);

    FragColor = texture(u_tex0, TextureCoordOut) * u_color * combinedColor;
#else
    FragColor = texture(u_tex0, TextureCoordOut) * u_color;
#endif

}
