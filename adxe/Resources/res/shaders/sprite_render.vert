#version 300 es

in vec3 a_position;
in vec2 a_texCoord;

out vec2 v_texCoord;

layout(std140) uniform vs_ub {
    uniform mat4 u_MVPMatrix;
};

void main()
{
//    gl_Position = (u_MVPMatrix * vec4(a_position, 1.0)).xyww;
    gl_Position = u_MVPMatrix * vec4(a_position, 1.0);
    v_texCoord = a_texCoord;
}
