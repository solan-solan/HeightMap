###################################################################################################
# Script was designed to prepare some skined model to be rendered with Axmol SkinBatch class
# It multipli's original object number of times to allow SkinBatch use them as separated objects, but render with 1 drawcall
# Created with Blender 3.3.1
# Usage:
# 1. Go to 'Scripting tab' in blender and open this script
# 2. Change 'create_patch' function parameters to desire
# 3. Do a copy of processing object before launch script, since it could not be restored
# 4. Add UVMap.001 (or another name) for the object [Object Data Properties->UV Maps->'+']. It should be VERTEX_ATTRIB_TEX_COORD1 attribute in axmol, since the shader will search the patch number here
# 5. Launch the script with 'Run Script' button. The script will duplicate the mesh with passed times and set corresponding patch number to the VERTEX_ATTRIB_TEX_COORD1 attribute for each vertex
#
# Good export to fbx:
# 1. Select 'model rig object' and corresponding mesh object
# 2. Choose File->Export->Fbx
# 3. Set Flag to 'Selected Objects'; choose Object types 'Armature' and 'Mesh'; Set 'Apply Scalings' to 'FBX All'; press 'Export FBX' to the relevant folder 
# 4. Process Fbx file with fbx-conv.exe tool to get *.c3b or *.c3t [ fbx-conv.exe -v -b -n 60 *.fbx ]
#
# !!! If Axmol 'Index buffer' uses 'unsigned short' internal type, there could be 0...65535 indicies and you should keep this in mind when select 'number of patch' parameter for 'create_patch' function
# Otherwise you will see bad rendering of SkinBatch object

from mathutils import *
import bpy


# Set 'num' to the VERTEX_ATTRIB_TEX_COORD1 attribute
def set_patch_num(m, num):
    
    for poly in m.polygons:
        for loop_index in range(poly.loop_start, poly.loop_start + poly.loop_total):

            #v_idx = m.loops[loop_index].vertex_index
            m.uv_layers[1].data[loop_index].uv[0] = num
            m.uv_layers[1].data[loop_index].uv[1] = num


# Check if 'num' was set properly for mesh
def check_num(m, num):
    
    print('num == ' + str(num))
    for poly in m.polygons:
        for loop_index in range(poly.loop_start, poly.loop_start + poly.loop_total):

            if  m.uv_layers[1].data[loop_index].uv[0] != num or m.uv_layers[1].data[loop_index].uv[1] != num:
                print('ERROR: not equal')
                return False
    
    print('SUCCESS')
    return True
    

# Create 'count_copy' pathces for the 'o' skined object
# The patch number will be set to the VERTEX_ATTRIB_TEX_COORD1 attribute
def create_patch(o, count_copy):
    
    bpy.ops.object.select_all(action='DESELECT')
    
    num = 0.0
    set_patch_num(o.data, num)
    if check_num(o.data, num) == True:
        
        for i in range(count_copy):
            num += 1.0
        
            n_o = o.copy()
            n_o.data = o.data.copy()
            set_patch_num(n_o.data, num)
            if check_num(n_o.data, num) == False:
                break
            
            bpy.context.collection.objects.link(n_o)
            n_o.select_set(True)
    
    if num == count_copy:
        o.select_set(True)
        bpy.context.view_layer.objects.active = o
        bpy.ops.object.join()
        print('ALL patches were created')
    else:
        print('ERROR: created only ' + str(num) + ' pathes')


# Launch
create_patch(bpy.data.objects.get('hen.mesh'), 49) # 1 itself + 49 duplicated == 50 patches
