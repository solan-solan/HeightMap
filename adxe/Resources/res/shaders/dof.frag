#version 300 es

precision highp float;

out vec4 FragColor;
in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform sampler2D u_texture_depth;

layout(std140) uniform fs_ub {
    uniform vec4 u_texel_clipplane;
    uniform int u_blur_radius;
    uniform float u_coef_dist;
};

float sample_linear_depth(vec2 texC)
{
    float fDepth = texture( u_texture_depth, texC ).x;

    return /*0.5 **/ u_texel_clipplane.z * u_texel_clipplane.w / 
        ( u_texel_clipplane.w - fDepth * (u_texel_clipplane.w - u_texel_clipplane.z) );
}

void main()
{
    float fDepth = sample_linear_depth(v_texCoord.st); // get start point depth

    float fMultiplier = (float(u_blur_radius) + 1.0);
    vec4 vColor = texture(u_texture, v_texCoord.st) * fMultiplier; // get start point color

    for (int k = 1; k <= u_blur_radius; k++)
    {
        float fStep = float(k);

        float fScale = (1.0 + float(u_blur_radius) - fStep) / 1.0;
        
        // depth of next point in positive direction
        float fNextDepth = sample_linear_depth(v_texCoord.st + u_texel_clipplane.xy * fStep);
 
        // depth of nect point in negative direction
        float fPrevDepth = sample_linear_depth(v_texCoord.st - u_texel_clipplane.xy * fStep);
        
        // difference between fragments. Coeff is more then difference is less
        float fPrevDiff = 1.0;//min(1.0, 1.0 / (1.0e-7 + abs(fPrevDepth - fDepth)));
        float fNextDiff = 1.0;//min(1.0, 1.0 / (1.0e-7 + abs(fNextDepth - fDepth)));

        // Coeff to control blur with respect to depth
        float fDepthCoeff = (fDepth / u_texel_clipplane.w) * u_coef_dist;

        vColor += texture(u_texture, v_texCoord.st + u_texel_clipplane.xy * fStep ) * fScale * fNextDiff * fDepthCoeff;
        vColor += texture(u_texture, v_texCoord.st - u_texel_clipplane.xy * fStep ) * fScale * fPrevDiff * fDepthCoeff;

        // Calculate depth multiplier
        fMultiplier += fScale * (fPrevDiff + fNextDiff) * fDepthCoeff;
    };

    FragColor = vec4( vColor.xyz / fMultiplier, 1.0 );
}
