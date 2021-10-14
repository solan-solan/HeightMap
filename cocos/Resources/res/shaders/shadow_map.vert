attribute float a_height;
attribute float a_vertex_x;
attribute float a_vertex_z;

uniform mat4 u_VPMatrix;
#ifndef FIRST_LOD
    uniform vec3 u_camPos;
    uniform vec4 u_Ndraw;
    uniform vec3 u_lod_radius;
#endif
uniform vec3 u_scale;

void main()
{
    float x = a_vertex_x;
    float z = a_vertex_z;

#ifndef FIRST_LOD
    vec2 xz_ndraw = vec2(x, z);
    float dist_x = distance(x, u_camPos.x);
    float dist_z = distance(z, u_camPos.z);
    float lod_alpha_x = 1.0;
    float lod_alpha_z = 1.0;
    
    lod_alpha_x = (dist_x - u_lod_radius.z) / (u_lod_radius.x - u_lod_radius.z);
    lod_alpha_x = clamp(lod_alpha_x, 0.0, 1.0);
    lod_alpha_z = (dist_z - u_lod_radius.z) / (u_lod_radius.x - u_lod_radius.z);
    lod_alpha_z = clamp(lod_alpha_z, 0.0, 1.0);

    float h = a_height * u_scale.y - 100000.0 * float(all(bvec4(lessThan(xz_ndraw.xy, u_Ndraw.yw), greaterThan(xz_ndraw.xy, u_Ndraw.xz))));
#else
    float h = a_height * u_scale.y;
#endif

	vec3 pos = vec3(x, h, z);
    vec4 pp = u_VPMatrix * vec4(pos, 1.0);

#ifndef FIRST_LOD
//    pp.z -= u_lod_radius.y;
#endif

	gl_Position = pp;
}
