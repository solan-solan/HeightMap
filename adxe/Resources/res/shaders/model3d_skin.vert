#ifdef USE_NORMAL_MAPPING
#if (MAX_DIRECTIONAL_LIGHT_NUM > 0)
uniform vec3 u_DirLightSourceDirection[MAX_DIRECTIONAL_LIGHT_NUM];
#endif
#endif
#if (MAX_POINT_LIGHT_NUM > 0)
uniform vec3 u_PointLightSourcePosition[MAX_POINT_LIGHT_NUM];
#endif
#if (MAX_SPOT_LIGHT_NUM > 0)
uniform vec3 u_SpotLightSourcePosition[MAX_SPOT_LIGHT_NUM];
#ifdef USE_NORMAL_MAPPING
uniform vec3 u_SpotLightSourceDirection[MAX_SPOT_LIGHT_NUM];
#endif
#endif

attribute vec3 a_position;

attribute vec4 a_blendWeight;
attribute vec4 a_blendIndex;

attribute vec2 a_texCoord;
attribute vec2 a_texCoord1;

attribute vec3 a_normal;
#ifdef USE_NORMAL_MAPPING
attribute vec3 a_tangent;
attribute vec3 a_binormal;
#endif

// Uniforms
uniform mat4 u_PMatrix;
uniform sampler2D u_tex_f;
uniform float u_ft_size;
uniform float u_mesh_bone_num;

// Varyings
varying vec2 TextureCoordOut;

#ifdef USE_NORMAL_MAPPING
#if MAX_DIRECTIONAL_LIGHT_NUM
varying vec3 v_dirLightDirection[MAX_DIRECTIONAL_LIGHT_NUM];
#endif
#endif
#if MAX_POINT_LIGHT_NUM
varying vec3 v_vertexToPointLightDirection[MAX_POINT_LIGHT_NUM];
#endif
#if MAX_SPOT_LIGHT_NUM
varying vec3 v_vertexToSpotLightDirection[MAX_SPOT_LIGHT_NUM];
#ifdef USE_NORMAL_MAPPING
varying vec3 v_spotLightDirection[MAX_SPOT_LIGHT_NUM];
#endif
#endif

#ifndef USE_NORMAL_MAPPING
#if ((MAX_DIRECTIONAL_LIGHT_NUM > 0) || (MAX_POINT_LIGHT_NUM > 0) || (MAX_SPOT_LIGHT_NUM > 0))
varying vec3 v_normal;
#endif
#endif

int delta_patch;

vec4 readFT(int index)
{
    vec2 coord = vec2(mod(float(index), u_ft_size) / u_ft_size, float(index / int(u_ft_size)) / u_ft_size);
    return texture2D(u_tex_f, coord);
}

void getPositionAndNormal(out vec4 position, out vec3 normal, out vec3 tangent, out vec3 binormal)
{
    float blendWeight = a_blendWeight[0];

    int matrixIndex = delta_patch + int (a_blendIndex[0]) * 3;

    vec4 matrixPalette1 = readFT(matrixIndex) * blendWeight;
    vec4 matrixPalette2 = readFT(matrixIndex+1) * blendWeight;
    vec4 matrixPalette3 = readFT(matrixIndex+2) * blendWeight;

    blendWeight = a_blendWeight[1];
    if (blendWeight > 0.0)
    {
        matrixIndex = delta_patch + int(a_blendIndex[1]) * 3;

        matrixPalette1 += readFT(matrixIndex) * blendWeight;
        matrixPalette2 += readFT(matrixIndex+1) * blendWeight;
        matrixPalette3 += readFT(matrixIndex+2) * blendWeight;

        blendWeight = a_blendWeight[2];
        if (blendWeight > 0.0)
        {
            matrixIndex = delta_patch + int(a_blendIndex[2]) * 3;

            matrixPalette1 += readFT(matrixIndex) * blendWeight;
            matrixPalette2 += readFT(matrixIndex+1) * blendWeight;
            matrixPalette3 += readFT(matrixIndex+2) * blendWeight;

            blendWeight = a_blendWeight[3];
            if (blendWeight > 0.0)
            {
                matrixIndex = delta_patch + int(a_blendIndex[3]) * 3;

                matrixPalette1 += readFT(matrixIndex) * blendWeight;
                matrixPalette2 += readFT(matrixIndex+1) * blendWeight;
                matrixPalette3 += readFT(matrixIndex+2) * blendWeight;
            }
        }
    }

    vec4 p = vec4(a_position, 1.0);
    position.x = dot(p, matrixPalette1);
    position.y = dot(p, matrixPalette2);
    position.z = dot(p, matrixPalette3);
    position.w = p.w;

#if ((MAX_DIRECTIONAL_LIGHT_NUM > 0) || (MAX_POINT_LIGHT_NUM > 0) || (MAX_SPOT_LIGHT_NUM > 0))
    vec4 n = vec4(a_normal, 0.0);
    normal.x = dot(n, matrixPalette1);
    normal.y = dot(n, matrixPalette2);
    normal.z = dot(n, matrixPalette3);
#ifdef USE_NORMAL_MAPPING
    vec4 t = vec4(a_tangent, 0.0);
    tangent.x = dot(t, matrixPalette1);
    tangent.y = dot(t, matrixPalette2);
    tangent.z = dot(t, matrixPalette3);
    vec4 b = vec4(a_binormal, 0.0);
    binormal.x = dot(b, matrixPalette1);
    binormal.y = dot(b, matrixPalette2);
    binormal.z = dot(b, matrixPalette3);
#endif
#endif
}

void main()
{
    vec4 position;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    
    int patch_num = int(a_texCoord1.x);
    int patch_pixel_num = int(u_mesh_bone_num) * 3; // since there is 3 mat pallite rows for one bone
    delta_patch = (patch_pixel_num + 4 + 3) * patch_num; // 4 pixel's to keep Model Matrix, 3 pixel's to keep Normal Matrix

    getPositionAndNormal(position, normal, tangent, binormal);

    // Delta to the model matrix of the certain patch 
    int mat_mdl_idx = delta_patch + patch_pixel_num;
    // Delta to the normal matrix of the certain patch
    int mat_nrm_idx = mat_mdl_idx + 4;

    mat4 mdl_mat = mat4(readFT(mat_mdl_idx), readFT(mat_mdl_idx+1), readFT(mat_mdl_idx+2), readFT(mat_mdl_idx+3));
    mat3 nrm_mat = mat3(readFT(mat_nrm_idx).rgb, readFT(mat_nrm_idx+1).rgb, readFT(mat_nrm_idx+2).rgb);
    vec4 ePosition = mdl_mat * position;

#ifdef USE_NORMAL_MAPPING
    #if ((MAX_DIRECTIONAL_LIGHT_NUM > 0) || (MAX_POINT_LIGHT_NUM > 0) || (MAX_SPOT_LIGHT_NUM > 0))
        vec3 eTangent = normalize(nrm_mat * tangent);
        vec3 eBinormal = normalize(nrm_mat * binormal);
        vec3 eNormal = normalize(nrm_mat * normal);
    #endif

    #if (MAX_DIRECTIONAL_LIGHT_NUM > 0)
        for (int i = 0; i < MAX_DIRECTIONAL_LIGHT_NUM; ++i)
        {
            v_dirLightDirection[i].x = dot(eTangent, u_DirLightSourceDirection[i]);
            v_dirLightDirection[i].y = dot(eBinormal, u_DirLightSourceDirection[i]);
            v_dirLightDirection[i].z = dot(eNormal, u_DirLightSourceDirection[i]);
        }
    #endif

    #if (MAX_POINT_LIGHT_NUM > 0)
        for (int i = 0; i < MAX_POINT_LIGHT_NUM; ++i)
        {
            vec3 pointLightDir = u_PointLightSourcePosition[i].xyz - ePosition.xyz;
            v_vertexToPointLightDirection[i].x = dot(eTangent, pointLightDir);
            v_vertexToPointLightDirection[i].y = dot(eBinormal, pointLightDir);
            v_vertexToPointLightDirection[i].z = dot(eNormal, pointLightDir);
        }
    #endif

    #if (MAX_SPOT_LIGHT_NUM > 0)
        for (int i = 0; i < MAX_SPOT_LIGHT_NUM; ++i)
        {
            vec3 spotLightDir = u_SpotLightSourcePosition[i] - ePosition.xyz;
            v_vertexToSpotLightDirection[i].x = dot(eTangent, spotLightDir);
            v_vertexToSpotLightDirection[i].y = dot(eBinormal, spotLightDir);
            v_vertexToSpotLightDirection[i].z = dot(eNormal, spotLightDir);

            v_spotLightDirection[i].x = dot(eTangent, u_SpotLightSourceDirection[i]);
            v_spotLightDirection[i].y = dot(eBinormal, u_SpotLightSourceDirection[i]);
            v_spotLightDirection[i].z = dot(eNormal, u_SpotLightSourceDirection[i]);
        }
    #endif
#else
    #if (MAX_POINT_LIGHT_NUM > 0)
        for (int i = 0; i < MAX_POINT_LIGHT_NUM; ++i)
        {
            v_vertexToPointLightDirection[i] = u_PointLightSourcePosition[i].xyz- ePosition.xyz;
        }
    #endif

    #if (MAX_SPOT_LIGHT_NUM > 0)
        for (int i = 0; i < MAX_SPOT_LIGHT_NUM; ++i)
        {
            v_vertexToSpotLightDirection[i] = u_SpotLightSourcePosition[i] - ePosition.xyz;
        }
    #endif

    #if ((MAX_DIRECTIONAL_LIGHT_NUM > 0) || (MAX_POINT_LIGHT_NUM > 0) || (MAX_SPOT_LIGHT_NUM > 0))
        v_normal = nrm_mat * normal;
    #endif
#endif

    TextureCoordOut = a_texCoord;
    TextureCoordOut.y = 1.0 - TextureCoordOut.y;
    gl_Position = u_PMatrix * ePosition;
}
