
uniform vec4 u_color;

#ifdef GL_ES
    precision mediump float;

    #ifndef FIRST_LOD
        varying mediump float v_lod_alpha;
    #endif
#else

    #ifndef FIRST_LOD
        varying float v_lod_alpha;
    #endif
#endif

void main()
{
    vec4 color = u_color;

#ifndef FIRST_LOD
    color.a *= v_lod_alpha;
#endif

    gl_FragColor = color;
}
