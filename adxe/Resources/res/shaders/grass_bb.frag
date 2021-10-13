#ifdef GL_ES
    precision mediump float;
    #ifdef SHADOW
        varying mediump vec4 v_smcoord;
    #endif
    varying mediump vec2 v_texCoord;
    varying mediump vec3 v_normal;
    varying mediump float v_dist_alpha;
#else
    #ifdef SHADOW
        varying vec4 v_smcoord;
    #endif
    varying vec2 v_texCoord;
    varying vec3 v_normal;
    varying float v_dist_alpha;
#endif

uniform sampler2D u_texture;
uniform vec3 u_DirLight;
uniform vec3 u_DirColLight;
uniform vec3 u_AmbColLight;

#ifdef SHADOW
    uniform sampler2D u_text_sh;
    uniform vec2 u_texelSize_sh;
#endif

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

vec3 computeLighting(vec3 normalVector, vec3 lightDirection)
{
    float attenuation = max(dot(normalVector, lightDirection), 0.3);
    float diffuse = max(dot(vec3(0.0, 1.0, 0.0), lightDirection), 0.0);
    vec3 diffuseColor = u_DirColLight  * diffuse * attenuation;

    return diffuseColor;
}

void main(void)
{
	vec4 combinedColor = vec4(computeLighting(v_normal, -u_DirLight) + u_AmbColLight, 1.0);
	vec4 px_col = texture2D(u_texture, v_texCoord);
	if (px_col.a < 0.5) discard;

#ifdef SHADOW
    float shadow = shadowCalc();
    // Scale 0.0-1.0 to 0.2-1.0
    // otherways everything in shadow would be black
    shadow = (shadow * 0.8) + 0.2;
    px_col.rgb *= shadow;
#endif

    px_col.a *= v_dist_alpha;
	gl_FragColor = px_col * combinedColor;
}
