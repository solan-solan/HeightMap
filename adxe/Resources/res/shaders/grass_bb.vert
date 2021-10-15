attribute vec4 a_position;
attribute vec2 a_color;
attribute vec2 a_texCoord;

uniform vec3  u_up;
uniform vec3  u_right;

uniform mat4 u_VPMatrix;
uniform float u_time;

uniform float u_speed;
uniform vec3  u_scale;
uniform float u_size_min;
uniform float u_size_max;
uniform vec3 u_cam_pos;
uniform vec2 u_dist_alpha;

#ifdef SHADOW
    uniform mat4 u_lVP_sh[DEPTH_TEXT_COUNT];
    uniform float u_shadow_fade_dist;
#endif

#ifdef GL_ES
    #ifdef SHADOW
        varying mediump vec4 v_smcoord[DEPTH_TEXT_COUNT];
        varying mediump float v_shadow_fade_dist;
    #endif
    varying mediump vec2 v_texCoord;
    varying mediump vec3 v_normal;
    varying mediump float v_dist_alpha;
#else
    #ifdef SHADOW
        varying vec4 v_smcoord[DEPTH_TEXT_COUNT];
        varying float v_shadow_fade_dist;
    #endif
    varying vec2 v_texCoord;
    varying vec3 v_normal;
    varying float v_dist_alpha;
#endif

float local_rand(float seed, float max_val) // RAND_MAX assumed to be 'max_val'
{
    float val = seed * 1103515245.0 + 12345.0;
    return mod(val / 65536.0, max_val);
}

void main()
{
    float x = a_position.x;
    float z = a_position.z;

	vec4 pos = vec4(x, a_position.y * u_scale.y, z, 1.0);

    vec3 up = vec3(0.0, 1.0, 0.0);

    float seed = abs(a_color.x) + abs(a_color.y);
    float add_size = local_rand(mod(seed, 65536.0), u_size_max - u_size_min + 0.0001);
	vec4  vertex = pos + vec4 ( (a_texCoord.x - 0.5) * (u_size_min + add_size) * u_right + (1.0 - a_texCoord.y - 0.1) * (u_size_min + add_size) * up, 0.0 ) + vec4(a_color.x, 0.0, a_color.y, 0.0);

    v_texCoord = a_texCoord;

    float angle1 = u_time + 0.5077 * vertex.x - 0.421 * vertex.z;
    float angle2 = 1.31415 * u_time + 0.3 * vertex.y - 0.6 * vertex.z;

    vertex += (u_speed * vec4(sin(angle1), 0.0, cos(angle2), 0.0)) * (1.0 - a_texCoord.y);
    v_normal = normalize(u_cam_pos - vec3(vertex));

#ifdef SHADOW
    for (int i = 0; i < DEPTH_TEXT_COUNT; ++i)
        v_smcoord[i] = u_lVP_sh[i] * vertex;
#endif

    vertex = u_VPMatrix * vertex;

#ifdef SHADOW
    v_shadow_fade_dist = abs(vertex.z) / u_shadow_fade_dist;
    v_shadow_fade_dist = clamp(v_shadow_fade_dist, 0.0, 1.0);
#endif

    v_dist_alpha = 1.0 - (abs(vertex.z) - u_dist_alpha.x) / u_dist_alpha.y;
    v_dist_alpha = clamp(v_dist_alpha, 0.0, 1.0);

    gl_Position = vertex;    
}
