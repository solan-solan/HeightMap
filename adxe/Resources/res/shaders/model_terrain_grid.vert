attribute float a_height;
attribute float a_vertex_x;
attribute float a_vertex_z;

uniform mat4 u_VPMatrix;

uniform vec3 u_scale;
uniform vec4 u_Ndraw;
uniform vec3 u_lod_radius;
uniform vec3 u_camPos;

#ifdef GL_ES
    precision mediump float;
    
    varying mediump float v_lod_alpha;
    varying mediump float v_dist;
    varying mediump float v_noradius;
#else

    varying float v_lod_alpha;
    varying float v_dist;
    varying float v_noradius;
#endif

void main()
{
    float x = a_vertex_x;
    float z = a_vertex_z;

    vec2 xz_ndraw = vec2(x, z);

    v_dist = distance(vec2(x, z), vec2(u_camPos.x, u_camPos.z));
    v_lod_alpha = 1.0;
    v_noradius = u_lod_radius.z;
    if (u_lod_radius.z > 0.0)
    {
        v_lod_alpha = (v_dist - u_lod_radius.z) / (u_lod_radius.x - u_lod_radius.z);
        v_lod_alpha = clamp(v_lod_alpha, 0.0, 1.0);
    }

    float h = a_height * u_scale.y;

	vec3 pos = vec3(x, h, z);

    vec4 pp = u_VPMatrix * vec4(pos, 1.0);
    
    pp.z -= u_lod_radius.y;
	gl_Position = pp;
}
