import bpy
outfile = "Temp/blenderToGlb.glb" # TODO not hardcoded
bpy.ops.export_scene.gltf(filepath=outfile, check_existing=False, export_apply=True)
