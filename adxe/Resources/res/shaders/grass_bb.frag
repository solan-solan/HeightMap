
precision highp float;
precision highp int;

out vec4 FragColor;

#define vvec2_def(x, y) vec4 x[(y * 2 + 3) / 4]
#define vvec2_at1(x, y, z) x[(y / 2)][y % 2 * 2 + z]
#define vvec2_at(x, y) vec2(vvec2_at1(x, y, 0), vvec2_at1(x, y, 1))

#ifdef SHADOW
    in vec4 v_smcoord[DEPTH_TEXT_COUNT];
    in float v_shadow_fade_dist;
#endif
in vec2 v_texCoord;
in vec3 v_normal;
in float v_dist_alpha;
in float v_texIdx;

uniform sampler2D u_texture[GRASS_TEXT_COUNT];

#ifdef SHADOW
    uniform sampler2D u_text_sh[DEPTH_TEXT_COUNT]; 
#endif

layout(std140) uniform fs_ub {
    uniform vec3 u_DirLight;
    uniform vec3 u_DirColLight;
    uniform vec3 u_AmbColLight;

    #ifdef SHADOW
        vvec2_def(u_texelSize_sh, DEPTH_TEXT_COUNT);
        uniform float u_smooth_rate_sh;
        uniform float u_light_thr_sh; 
    #endif
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
	//vec4 px_col = texture(u_texture[int(step(0.5, v_texIdx))], v_texCoord);
    vec4 px_col = texture(u_texture[0], v_texCoord) * (1.0 - v_texIdx) + texture(u_texture[1], v_texCoord) * v_texIdx;
	if (px_col.a < 0.5) discard;

#ifdef SHADOW
    // If the fragment is not behind light view frustum
    float shadow = shadowCalc();
    // Scale 0.0-1.0 to 0.2-1.0
    // otherways everything in shadow would be black
    shadow = (shadow * (1.0 - u_light_thr_sh)) + u_light_thr_sh;
    shadow = mix(shadow, 1.0, v_shadow_fade_dist);
    px_col.rgb *= shadow;
#endif

    px_col.a *= v_dist_alpha;
	FragColor = px_col * combinedColor;
}
