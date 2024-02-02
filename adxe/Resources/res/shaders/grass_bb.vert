
in vec4 a_position;
in vec2 a_color;
in vec2 a_texCoord;
in float a_texIdx;
in float a_size;
in float a_patch_num;

layout(std140) uniform vs_ub {
    uniform vec3  u_up;
    uniform vec3  u_right;

    uniform mat4 u_VPMatrix;
    uniform float u_time;

    uniform float u_speed;
    uniform vec3  u_scale;
    uniform float u_patch_x_count;
    uniform float u_patch_y_count;
    uniform vec3 u_cam_pos;
    uniform vec2 u_dist_alpha;

    #ifdef SHADOW
        uniform mat4 u_lVP_sh[DEPTH_TEXT_COUNT];
        uniform float u_shadow_fade_dist;
    #endif
};

#ifdef SHADOW
    out vec4 v_smcoord[DEPTH_TEXT_COUNT];
    out float v_shadow_fade_dist;
#endif
out vec2 v_texCoord;
out vec3 v_normal;
out float v_dist_alpha;
out float v_texIdx;

void main()
{
    //v_texIdx = a_texIdx;
    v_texIdx = float(int(step(0.5, a_texIdx)));
    float x = a_position.x;
    float z = a_position.z;

	vec4 pos = vec4(x, a_position.y * u_scale.y, z, 1.0);

    vec3 up = vec3(0.0, 1.0, 0.0);

    vec4  vertex = pos + vec4 ( (a_texCoord.x - 0.5) * a_size * u_right + (1.0 - a_texCoord.y - 0.1) * a_size * up, 0.0 ) + vec4(a_color.x, 0.0, a_color.y, 0.0);

    // Texture coords calculation
    float scale_x_text = a_texCoord.x / u_patch_x_count;
    float scale_y_text = a_texCoord.y / u_patch_y_count;
    float col_num_text = mod(a_patch_num, u_patch_x_count);
    float row_num_text = float(int(a_patch_num) / int(u_patch_x_count));
    v_texCoord = vec2(scale_x_text + col_num_text * (1.0 / u_patch_x_count), scale_y_text + row_num_text * (1.0 / u_patch_y_count));

    // Grass move calculation
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
