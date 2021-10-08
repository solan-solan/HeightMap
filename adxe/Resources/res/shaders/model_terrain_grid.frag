
uniform vec4 u_color;

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
    if (v_dist < v_noradius)
        discard;

    vec4 color = u_color;
    color.a *= v_lod_alpha;

    gl_FragColor = color;
}
