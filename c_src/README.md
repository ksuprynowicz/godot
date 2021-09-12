# Unifex Godot Engine

```bash
sudo apt install -y elixir erlang-dev
mix deps.get
mix compile
```

Launch.

Create a script in the fs.


```
extends SceneTree

func _init():
	var gltf_doc = GLTFDocument.new()
	var packed_scene = PackedScene.new()
	var node = gltf_doc.import_scene("Fox.glb")
	packed_scene.pack(node)
	print("test")
	ResourceSaver.save("saved.tscn", packed_scene)
	quit()
```

```bash
iex -S mix
require Unifex.CNode
{:ok, pid} = Unifex.CNode.start_link(:godot_elixir)
Unifex.CNode.call(pid, :init, [["godot", "--verbose", "--headless"]])
Unifex.CNode.call(pid, :call, ["get_node_count"])
Unifex.CNode.call(pid, :call, ["get_method_list"])
Unifex.CNode.call(pid, :set_gdscript, [script])
Unifex.CNode.stop(pid)
```

Unifex.CNode.call(pid, :init, [["/nexus/V-Sekai/workspace/godot/bin/godot", "--verbose", "--headless", "--script", "./new_script.gd"]])

