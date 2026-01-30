from pygltflib import GLTF2

gltf = GLTF2().load("../resource/assets/Avatar02.glb")

for mi, mesh in enumerate(gltf.meshes or []):
    print("Mesh", mi, mesh.name)
    for ti, target in enumerate(mesh.primitives[0].targets or []):
        print("  Target", ti, target)  # 여기서 POSITION/NORMAL 덩어리, 이름이 없을 수도 있음