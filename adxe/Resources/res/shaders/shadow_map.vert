
in float a_height;
in float a_vertex_x;
in float a_vertex_z;

layout(std140) uniform vs_ub {
    uniform mat4 u_VPMatrix;
    #ifndef FIRST_LOD
        uniform vec4 u_Ndraw;
    #endif
    uniform vec3 u_scale;
};

void main()
{
    float x = a_vertex_x;
    float z = a_vertex_z;

#ifndef FIRST_LOD
    vec2 xz_ndraw = vec2(x, z);
    float h = a_height * u_scale.y - 100000.0 * float(all(bvec4(lessThan(xz_ndraw.xy, u_Ndraw.yw), greaterThan(xz_ndraw.xy, u_Ndraw.xz))));
#else
    float h = a_height * u_scale.y;
#endif

	vec3 pos = vec3(x, h, z);
    vec4 pp = u_VPMatrix * vec4(pos, 1.0);

	gl_Position = pp;
}
