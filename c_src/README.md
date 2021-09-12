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

```elixir
iex -S mix
{:ok, pid} = Godot.start_link(["--verbose", "--headless", "--path", "/nexus/V-Sekai/dance", "-e"])
{:ok, pid} = Godot.start_link(["--verbose"])
GenServer.call(pid, {:call, [["get_method_list", nil, nil, nil, nil, nil]]})
GenServer.call(pid, {:call, [["get_node_count", nil, nil, nil, nil, nil]]})
GenServer.stop(pid)
```