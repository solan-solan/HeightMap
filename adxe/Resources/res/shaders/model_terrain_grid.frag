
precision highp float;

out vec4 FragColor;

layout(std140) uniform fs_ub {
    uniform vec4 u_color;
};

#ifndef FIRST_LOD
    in float v_lod_alpha;
#endif


void main()
{
    vec4 color = u_color;

#ifndef FIRST_LOD
    color.a *= v_lod_alpha;
#endif

    FragColor = color;
}
