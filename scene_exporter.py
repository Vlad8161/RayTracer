import bpy
import bmesh
import os
import json

def export_scene():
    project_name = bpy.path.basename(bpy.context.blend_data.filepath).strip('.blend')
    project_path = bpy.path.abspath('//')
    export_path = project_path + '/' + project_name + '.scene'

    result = {
        'vertices': [],
        'faces': [],
        'spheres': [],
        'materials': [],
        'lamps': [],
    }

    if len(bpy.data.worlds) > 0:
        result['world'] = {
                'ambientColor': [float(i) for i in bpy.data.worlds[0].ambient_color],
                'ambientFactor': bpy.data.worlds[0].light_settings.ao_factor,
                'horizonColor': [float(i) for i in bpy.data.worlds[0].horizon_color],
        }
    else:
        result['world'] = None

    for i in bpy.data.objects:
        if i.hide_render:
            continue

        if i.type == 'MESH':
            bm = bmesh.new()
            bm.from_mesh(i.data)
            bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method=0, ngon_method=0)
            bm.to_mesh(i.data)
            bm.free()

            base_vertex_index = len(result['vertices'])

            if i.active_material is not None:
                texture = i.active_material.active_texture
                texture_slot = i.active_material.texture_slots[i.active_material.active_texture_index]
                try:
                    if texture.type == 'IMAGE':
                        image_path = texture.image.filepath_from_user()
                        scale_x = float(texture_slot.scale[0])
                        scale_y = float(texture_slot.scale[1])
                    else:
                        image_path = None
                except AttributeError:
                    image_path = None

                result['materials'].append({
                        'diffusiveFactor': i.active_material.diffuse_intensity,
                        'diffusiveColor': [float(i) for i in i.active_material.diffuse_color],
                        'specularFactor': i.active_material.specular_intensity,
                        'specularHardness': i.active_material.specular_hardness,
                        'reflectionFactor': i.active_material.raytrace_mirror.reflect_factor if i.active_material.raytrace_mirror.use else 0.0,
                        'imagePath': image_path,
                        'scaleX': scale_x,
                        'scaleY': scale_y,
                })
                material_index = len(result['materials']) - 1
            else:
                material_index = None

            mat = i.matrix_world
            for j in i.data.vertices:
                result['vertices'].append([float(k) for k in (mat * j.co)[:]])

            uv_layer = i.data.uv_layers.active
            for j in i.data.polygons:
                uv = None
                if uv_layer is not None:
                    uv = []
                    for li in range(j.loop_start, j.loop_start + j.loop_total):
                        uv.append([float(k) for k in uv_layer.data[li].uv])

                result['faces'].append({
                    'vertices': [k + base_vertex_index for k in j.vertices],
                    'material': material_index,
                    'uv': uv,
                })

        if i.type == 'CAMERA':
            mat = i.matrix_world
            cam_transform_mat = (
                (mat[0][0], mat[0][1], mat[0][2]),
                (mat[1][0], mat[1][1], mat[1][2]),
                (mat[2][0], mat[2][1], mat[2][2]),
            )
            cam_translation_vec = (
                mat[0][3], 
                mat[1][3], 
                mat[2][3], 
            )
            result['transform'] = cam_transform_mat
            result['translate'] = cam_translation_vec

        if i.type == 'LAMP':
            mat = i.matrix_world
            cam_translation_vec = (
                mat[0][3], 
                mat[1][3], 
                mat[2][3], 
            )
            result['lamps'].append({
                'pos': cam_translation_vec,
                'intensity': i.data.energy,
                'distance': i.data.distance,
            })

        if i.type == 'META':
            if i.active_material is not None:
                result['materials'].append({
                        'diffusiveFactor': i.active_material.diffuse_intensity,
                        'diffusiveColor': [float(i) for i in i.active_material.diffuse_color],
                        'reflectionFactor': i.active_material.raytrace_mirror.reflect_factor if i.active_material.raytrace_mirror.use else 0.0,
                        'specularFactor': i.active_material.specular_intensity,
                        'specularHardness': i.active_material.specular_hardness,
                })
                material_index = len(result['materials']) - 1
            else:
                material_index = None

            mat = i.matrix_world
            for j in i.data.elements:
                if j.type == 'BALL':
                    result['spheres'].append({
                        'center': [float(k) for k in mat * j.co],
                        'radius': j.radius / 2,
                        'material': material_index,
                    })
                
    with open(export_path, 'wt') as f:
        f.write(json.dumps(result, indent=4))

export_scene()
