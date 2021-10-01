#ifdef GL_ES
    precision mediump float;
    varying mediump vec2 v_texCoord;
    varying mediump vec3 v_normal;
    varying mediump float v_dist_alpha;
#else
    varying vec2 v_texCoord;
    varying vec3 v_normal;
    varying float v_dist_alpha;
#endif

uniform sampler2D u_texture;
uniform vec3 u_DirLight;
uniform vec3 u_DirColLight;
uniform vec3 u_AmbColLight;

vec3 computeLighting(vec3 normalVector, vec3 lightDirection)
{
    //float attenuation = 1.0;

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
    px_col.a *= v_dist_alpha;
	gl_FragColor = px_col * combinedColor;
}
