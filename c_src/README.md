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
{:ok, state} = Godot.init(["--help"])
{:ok, state} = Godot.call(state, ["get_node_count"])
{:ok, state} = Godot.call(state, ["get_method_list"])
{:ok, state} = Godot.stop(state)
```

