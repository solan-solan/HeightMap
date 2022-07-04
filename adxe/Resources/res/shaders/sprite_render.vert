
attribute vec3 a_position;
attribute vec2 a_texCoord;

#ifdef GL_ES
varying mediump vec2 v_texCoord;
#else
varying vec2 v_texCoord;
#endif

uniform mat4 u_MVPMatrix;

void main()
{
//    gl_Position = (u_MVPMatrix * vec4(a_position, 1.0)).xyww;
    gl_Position = u_MVPMatrix * vec4(a_position, 1.0);
    v_texCoord = a_texCoord;
}
