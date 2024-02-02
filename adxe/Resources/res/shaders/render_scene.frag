#version 300 es

precision highp float;

out vec4 FragColor;

uniform sampler2D u_texture;
in vec2 v_texCoord;

void main()
{
    FragColor = texture(u_texture, v_texCoord);
}
