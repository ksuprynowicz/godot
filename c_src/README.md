# Unifex Godot Engine

```bash
sudo apt install -y elixir erlang-dev
mix deps.get
mix compile
```

Launch.

```
script = "\
extends Node
func _init():  \
    var gltf_doc = GLTFDocument.new()  \
    var packed_scene = PackedScene.new() \
    var node = gltf_doc.import_scene(\"res://Fox.glb\") \
    packed_scene.pack(node) \
    ResourceSaver.save(\"res://ran.scn\", packed_scene, ResourceSaver.FLAG_COMPRESS)"
```

```bash
iex -S mix
require Unifex.CNode
{:ok, pid} = Unifex.CNode.start_link(:godot_elixir)
Unifex.CNode.call(pid, :init, [["godot", "-v", "--headless"]])
Unifex.CNode.call(pid, :iteration, [0.033])
Unifex.CNode.call(pid, :call, ["get_node_count"])
Unifex.CNode.call(pid, :call, ["get_method_list"])
Unifex.CNode.call(pid, :init, [["godot", "-v"]])
Unifex.CNode.stop(pid)
```