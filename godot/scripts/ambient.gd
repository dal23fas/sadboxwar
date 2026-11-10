# EFAmbient — presentation-only ambient life (Phase 6 placeholder): fishing boats on the sea lanes,
# civilian cars on roads, villagers strolling near POIs. No world-state involvement.
class_name EFAmbient
extends Node3D

var _movers: Array = []     # {node, a: Vector3, b: Vector3, t, speed, dir}
var _terrain: EFTerrain

func build(content: Dictionary, terrain: EFTerrain) -> void:
	_terrain = terrain
	var poi_by_id := {}
	for p in content.get("pois", []):
		poi_by_id[p["id"]] = World.to_godot(p["pose"]["position"])
	for lane in content.get("terrain", {}).get("sea_lanes", []):
		if poi_by_id.has(lane[0]) and poi_by_id.has(lane[1]):
			var a: Vector3 = poi_by_id[lane[0]]; var b: Vector3 = poi_by_id[lane[1]]
			a.y = 0.0; b.y = 0.0
			_add_mover(_make_visual("naval", "Fishing boat"), _offshore(a, b), _offshore(b, a), 4.0)
	for road in content.get("terrain", {}).get("roads", []).slice(0, 4):
		if poi_by_id.has(road[0]) and poi_by_id.has(road[1]):
			_add_mover(_make_visual("ground", "Civilian car"), poi_by_id[road[0]], poi_by_id[road[1]], 11.0, true)
	for p in content.get("pois", []):
		if p.get("kind", "") in ["town", "hamlet"]:
			var c: Vector3 = poi_by_id[p["id"]]
			for i in range(3):
				var a := c + Vector3(randf_range(-30, 30), 0, randf_range(-30, 30))
				var b := c + Vector3(randf_range(-30, 30), 0, randf_range(-30, 30))
				_add_mover(_make_villager(), a, b, 1.2, true)

func _offshore(a: Vector3, toward: Vector3) -> Vector3:
	return a + (toward - a).normalized() * 60.0

func _make_visual(vclass: String, label: String) -> Node3D:
	var n := Node3D.new()
	add_child(n)
	VehicleVisuals.build("ambient_" + vclass, vclass, n)
	var l := Label3D.new(); l.text = label; l.position.y = 4.0; l.pixel_size = 0.02; l.font_size = 32; l.billboard = BaseMaterial3D.BILLBOARD_ENABLED; l.modulate = Color(0.8, 0.8, 0.8)
	n.add_child(l)
	return n

func _make_villager() -> Node3D:
	var n := Node3D.new()
	add_child(n)
	var mi := MeshInstance3D.new(); var cm := CapsuleMesh.new(); cm.radius = 0.3; cm.height = 1.7; mi.mesh = cm; mi.position.y = 0.85
	var mat := StandardMaterial3D.new(); mat.albedo_color = Color(randf_range(0.3, 0.8), randf_range(0.3, 0.7), randf_range(0.3, 0.7)); mi.material_override = mat
	n.add_child(mi)
	return n

func _add_mover(node: Node3D, a: Vector3, b: Vector3, speed: float, on_terrain := false) -> void:
	node.set_meta("on_terrain", on_terrain)
	_movers.append({"node": node, "a": a, "b": b, "t": randf(), "speed": speed, "dir": 1.0})
	node.global_position = a

func _process(delta: float) -> void:
	for m in _movers:
		var a: Vector3 = m["a"]; var b: Vector3 = m["b"]
		var length: float = max(a.distance_to(b), 1.0)
		m["t"] += m["dir"] * delta * m["speed"] / length
		if m["t"] >= 1.0:
			m["t"] = 1.0; m["dir"] = -1.0
		elif m["t"] <= 0.0:
			m["t"] = 0.0; m["dir"] = 1.0
		var node: Node3D = m["node"]
		var target: Vector3 = (b if m["dir"] > 0 else a)
		var pos: Vector3 = a.lerp(b, m["t"])
		if node.get_meta("on_terrain", false):
			pos.y = _terrain.height_at(pos.x, pos.z) + 0.05
		node.global_position = pos
		if pos.distance_to(target) > 0.5:
			node.look_at(Vector3(target.x, pos.y, target.z), Vector3.UP)
