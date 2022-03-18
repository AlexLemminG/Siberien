import bpy
import os

outfile = os.getenv('SENGINE_BLENDER_EXPORTER_OUTPUT_FILE')
animateDeformBonesOnly = os.getenv('SENGINE_BLENDER_EXPORTER_ANIMATIONS_DEFORM_BONES_ONLY') == "True"

bpy.ops.export_scene.gltf(filepath=outfile, check_existing=False, export_apply=True, export_def_bones=animateDeformBonesOnly)
