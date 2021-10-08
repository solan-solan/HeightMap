
uniform vec4 u_color;

#ifdef GL_ES
    precision mediump float;
    
    varying mediump float v_lod_alpha;
#else

    varying float v_lod_alpha;
#endif

void main()
{
    vec4 color = u_color;
    color.a *= v_lod_alpha;

    gl_FragColor = color;
}
