attribute float a_height;
attribute float a_vertex_x;
attribute float a_vertex_z;

uniform mat4 u_VPMatrix;

uniform float u_width;
uniform float u_height;
uniform vec3 u_scale;
uniform vec4 u_Ndraw;


void main()
{
    float x = a_vertex_x;
    float z = a_vertex_z;

    vec2 xz_ndraw = vec2(x, z);

    float h = a_height * u_scale.y - 10000.0 * float(all(bvec4(lessThan(xz_ndraw.xy, u_Ndraw.yw), greaterThan(xz_ndraw.xy, u_Ndraw.xz))));

	vec3 pos = vec3(x, h, z);

    vec4 pp = u_VPMatrix * vec4(pos, 1.0);

	gl_Position = pp;
}
