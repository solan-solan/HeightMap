# HeightMap
HeightMap ClipMaps CPU implementation for ADXE and Cocos2d v4, Gles 2.0 supports 

This repository includes two examples for ADXE and Coso2dx v4.

1. Cocos ex.: You need to add 'cocos2d' folder with the engine itself to the 'cocos' folder

2. ADXE ex.: You need to have ADXE_ROOT environment varriable with the path to the ADXE engine 

HeightMap supports the following features:
- Infinity land;
- NormalMap;
- Multilayers (up to 4 diffuse textures and normal map textures on layer);
- Texture LOD's (Each HeightMap LOD renderes the textures with its own size. There is mechanic to smooth passing from LOD to LOD for textures.)
- Any number of LOD's;
- Exists base grass implementation;
- All loaded datas about heights will be stored to the HeightChunks to save memory;
- You can load any number of 'c3b' files with the Land representation. It is all affect's only operating memory. The number of rendered verticies is related only from the LOD's size and LOD's count.
- Property file to adjust HeigthMap (res/hm/prop);
- Distant Fog.
- Get height of the land according to the (X, Z) world coords;
- Touch Ray cast of the first LOD;

Look please at the example code, how to use HeightMap.

The main idea is to have opportunity to create whole 3d level in the Blender (2.8 or later) or some other 3d applications, which can include not only the ground but the other static objects also, since there is no level 3d editors for the corresponding engines.
It will help you to create big land mass with objects on it.
(Of cause it is unlikely help to create morrowind like games, but may be DAO style).

The Workflow could be the following;
- Create 'plane' in the Blender. 
- Create 'multires' modificator for the 'plane' to subdivide it on any number of poligons   (look heights_text.blend example).
- Draw heights with the internal sculpting tool (IMPORTANT: It is needed to lock vertix changing for X and Y axis in the sculpting settings. Another words the scale between the verticies alog X and Y axis should be the same).
- Apply modificator to create mesh.
- Draw Map png file using red, blue, green and alpha chanel's. It could be done in any drawing tool. Each of the channel will represent the rate for the corresponding texture (as it done in cocos2d).
    There is correspondence between color channels and HeightMap property file
    Red channel - "text_r"
    Green channel - "text_g"
    Blue channel - "text_b"
    1.0 - Alpha channel - "text_a" (Alpha channel is represented as 'erased'. In other words you should erase places in the editors where "text_a" should be. For blender it is needed to change 'brush blending mode' from Mix to Erase Alpha.)

(If about blender, you can find the blender shader in the heights_text.blend which allows you to correspond each color channel to some texture).
Each map image satisfy to the layer in terms of HeightMap. Alpha channel of the first layer is the base texture for all HeightMap default places.
(You should leave alpha channel as 1.0 (black color) for other than first layers if do not want that they overlap default texture). As in example.
- Place other game objects in the same blender file on the place where they should be at the game (like house in the example).
(You can combine this objects if they are static).
- After that you should export HeightMap (remove uv attribute to save memory) and game objects to the separate FBX files, using export setings (Limit to: selected object, Apply scalings: FBX All).
- Convert all files from FBX to c3b;
- Create HeightMap and other objects in the game code like it done in the example.

There are many other ideas how this code can be adapted to use (from the animated ocean to the in game minimap).
It is not straightforward ClipMaps implementation. For example there is only vertex buffer updating (no index updating).

Of cause you do not find here some teselation decision. Besides, it could be visible as Lods changing (even with texture LODS).
It relates with texture Map image (How it was drawing and its resolution), and how strong HeightMap polygons were stretched.
Was found one issue on some graphic cards - thin flickering line on the border of two LODs. (I do not see it on the Intel)

Property description:

"lod_data" - section to describe LOD data
    "w_h" - vertex count along X and Z axis (2^n + 1)
    "text_lod_dist_from" - Texture LOD distance start. Texture will have own size before this distance.
        Should be farther than previouse LOD is ended. LOD End calculation: (w_h / 2.0) * lod_scale_x_z * 2^(lod_num-1)
    "text_lod_dist_to" - Texture LOD distance end. Texture will have size of the next LOD after. Should be less than end of the current LOD. Calculated the same as before.
    (Texture LOD disable if '0.0')
"layers" - section to describe layers data
    "dist_view" - Distance where the textures of the layer will be visible. It can be calculated how many
        LODs will be drawn for the layer using this distance and LOD End calculation formula. (Works starting from the second layer).

        "text_x" - section to describe texture
            "diffuse" - diffuse texture path
            "normal" - normal map texture path
            "first_lod_size" - size of the texture for the first LOD (Should be chosen base on lod_scale_x_z)
            "scale_size_coef" - Coefficient to calculate next LOD size with the formula first_lod_size * scale_size_coef * lod_num
            "normal_map_scale" - NormalMap size
            "specular_factor" - Specular factor for the texture
"lod_chunk_count_w_h" - Count of chunks in the 'basket'. Basket keeps chunks which could be empty. It will be returned default height for the empty chunk. (According to this infinity has been reached).
"lod_chunk_size" - Size of one chunk (lod_chunk_size * lod_chunk_size memory under heights will be allocated even one height will be changed from the chunk)
     It is needed that lod_chunk_count_w_h * lod_chunk_size == 2^n+1
"lod_scale_x_z" - scale between vertex along X and Z axis.
"lod_scale_y" - scale along Y axis
"darker_dist" - distance where ground become darker (may be any)
"normal_map_en" - is normal map enable
"grass" - section to describe grass
    "rate" - How many grass patch is in one HeightMap tile. (Tile belongs to the (-X, -Z) vertex on the grid).
    "tile_count_coef" - How many tiles will be occupied by the grass. tile count == (tile_count_coef * 2 + 1) * (tile_count_coef * 2 + 1)
    "shift" - max shift of the grass patch inside the tile (Should be chosen according to the lod_scale_x_z).
    "speed" - speed of the grass animation
    "size_min" - min size of the grass patch
    "size_max" - max size of the grass patch

    All vertex coords will be aligned to the 1 glsl point and multiplied on lod_scale_x_z (should be integer).