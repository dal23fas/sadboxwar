# EFStructures — placeholder buildings per POI kind (towns, ports, airfields, clinic, depot, mine, hamlet).
# Everything is procedural primitives sitting on the terrain pads; replaced by real assets in Phase 8.
class_name EFStructures

static func _box(parent: Node3D, pos: Vector3, size: Vector3, color: Color, yaw_deg := 0.0, solid := true) -> StaticBody3D:
	var body := StaticBody3D.new()
	body.position = pos
	body.rotation_degrees.y = yaw_deg
	var mi := MeshInstance3D.new()
	var bm := BoxMesh.new()
	bm.size = size
	mi.mesh = bm
	mi.position.y = size.y * 0.5
	var mat := StandardMaterial3D.new()
	mat.albedo_color = color
	mat.roughness = 0.85
	mi.material_override = mat
	body.add_child(mi)
	if solid:
		var col := CollisionShape3D.new()
		var sh := BoxShape3D.new()
		sh.size = size
		col.shape = sh
		col.position.y = size.y * 0.5
		body.add_child(col)
	parent.add_child(body)
	return body

static func _cyl(parent: Node3D, pos: Vector3, radius: float, height: float, color: Color) -> void:
	var mi := MeshInstance3D.new()
	var cm := CylinderMesh.new()
	cm.top_radius = radius
	cm.bottom_radius = radius
	cm.height = height
	mi.mesh = cm
	mi.position = pos + Vector3(0, height * 0.5, 0)
	var mat := StandardMaterial3D.new()
	mat.albedo_color = color
	mi.material_override = mat
	parent.add_child(mi)

static func build_poi(parent: Node3D, poi: Dictionary, terrain: EFTerrain, rng: RandomNumberGenerator) -> void:
	var c: Vector3 = World.to_godot(poi["pose"]["position"])
	var kind: String = poi.get("kind", "")
	var ground := terrain.height_at(c.x, c.z)
	match kind:
		"town":
			var n := 14
			for i in range(n):
				var ang := TAU * i / n + rng.randf_range(-0.2, 0.2)
				var r := rng.randf_range(28.0, 70.0)
				var p := Vector3(c.x + cos(ang) * r, 0, c.z + sin(ang) * r)
				p.y = terrain.height_at(p.x, p.z)
				var size := Vector3(rng.randf_range(7, 13), rng.randf_range(5, 14), rng.randf_range(7, 13))
				var shade := rng.randf_range(0.55, 0.8)
				_box(parent, p, size, Color(shade, shade * 0.95, shade * 0.85), rad_to_deg(ang))
			_box(parent, Vector3(c.x, ground, c.z - 18), Vector3(16, 9, 12), Color(0.85, 0.82, 0.75))   # town hall
			_cyl(parent, Vector3(c.x + 12, ground, c.z - 18), 0.4, 14, Color(0.3, 0.3, 0.3))             # flag mast
		"port":
			_box(parent, Vector3(c.x, ground - 1.0, c.z), Vector3(60, 1.2, 24), Color(0.45, 0.43, 0.4))   # quay
			_box(parent, Vector3(c.x - 18, ground, c.z - 8), Vector3(14, 6, 9), Color(0.6, 0.55, 0.5))    # warehouse
			for i in range(5):
				_box(parent, Vector3(c.x + 5 + i * 6.5, ground, c.z - 6), Vector3(6, 2.6, 2.4), [Color(0.7, 0.2, 0.2), Color(0.2, 0.4, 0.7), Color(0.75, 0.6, 0.2)][i % 3])
			_cyl(parent, Vector3(c.x + 20, ground, c.z + 4), 0.6, 22, Color(0.85, 0.6, 0.15))             # crane mast
			_box(parent, Vector3(c.x + 20, ground + 21, c.z + 4), Vector3(1.2, 1.2, 26), Color(0.85, 0.6, 0.15), 0.0, false)   # crane arm
		"airfield":
			_box(parent, Vector3(c.x, ground - 0.9, c.z), Vector3(700, 1.0, 30), Color(0.28, 0.28, 0.3))  # runway
			for i in range(12):
				_box(parent, Vector3(c.x - 300 + i * 55, ground + 0.1, c.z), Vector3(18, 0.12, 1.0), Color(0.9, 0.9, 0.85), 0.0, false)   # centreline
			_box(parent, Vector3(c.x - 60, ground, c.z + 60), Vector3(34, 11, 30), Color(0.6, 0.62, 0.65))  # hangar
			_box(parent, Vector3(c.x + 80, ground, c.z + 55), Vector3(8, 14, 8), Color(0.75, 0.75, 0.78))   # tower base
			_box(parent, Vector3(c.x + 80, ground + 14, c.z + 55), Vector3(11, 4, 11), Color(0.4, 0.55, 0.7), 0.0, false)
			_box(parent, Vector3(c.x + 40, ground, c.z + 50), Vector3(20, 2.5, 6), Color(0.85, 0.85, 0.85))   # fuel bowser
		"clinic":
			_box(parent, Vector3(c.x, ground, c.z - 20), Vector3(26, 7, 16), Color(0.92, 0.92, 0.9))
			_box(parent, Vector3(c.x, ground + 7, c.z - 20), Vector3(4, 0.4, 1.2), Color(0.85, 0.15, 0.15), 0.0, false)
			_box(parent, Vector3(c.x, ground + 7, c.z - 20), Vector3(1.2, 0.4, 4), Color(0.85, 0.15, 0.15), 0.0, false)
			_box(parent, Vector3(c.x + 22, ground - 0.4, c.z + 10), Vector3(22, 0.5, 22), Color(0.3, 0.3, 0.32))   # helipad
			_box(parent, Vector3(c.x + 22, ground + 0.1, c.z + 10), Vector3(14, 0.1, 2), Color(0.95, 0.95, 0.2), 0.0, false)
			_box(parent, Vector3(c.x + 22, ground + 0.1, c.z + 10), Vector3(2, 0.1, 14), Color(0.95, 0.95, 0.2), 0.0, false)
		"depot":
			_box(parent, Vector3(c.x - 20, ground, c.z - 14), Vector3(30, 8, 18), Color(0.55, 0.5, 0.45))     # warehouse
			for i in range(3):
				_cyl(parent, Vector3(c.x + 22, ground, c.z - 16 + i * 10), 3.0, 6.0, Color(0.8, 0.8, 0.82))   # fuel tanks
			_box(parent, Vector3(c.x, ground - 0.4, c.z + 10), Vector3(70, 0.5, 40), Color(0.3, 0.3, 0.32))   # apron
		"industrial":
			_box(parent, Vector3(c.x, ground, c.z - 20), Vector3(10, 18, 10), Color(0.5, 0.45, 0.4))          # head frame
			_box(parent, Vector3(c.x + 20, ground, c.z - 10), Vector3(40, 6, 12), Color(0.55, 0.5, 0.45), -20)  # conveyor shed
			_box(parent, Vector3(c.x - 25, ground, c.z + 8), Vector3(18, 5, 12), Color(0.6, 0.55, 0.5))
			for i in range(4):
				_cyl(parent, Vector3(c.x + 30 + i * 7, ground, c.z + 16), 2.2, 2.0 + i, Color(0.4, 0.38, 0.35))   # spoil heaps
		"hamlet":
			for i in range(5):
				var ang := TAU * i / 5.0
				_box(parent, Vector3(c.x + cos(ang) * 22, terrain.height_at(c.x + cos(ang) * 22, c.z + sin(ang) * 22), c.z + sin(ang) * 22), Vector3(6, 4, 6), Color(0.7, 0.6, 0.45), rad_to_deg(ang))
			_box(parent, Vector3(c.x, ground - 0.5, c.z + 40), Vector3(18, 0.8, 6), Color(0.5, 0.45, 0.4))   # jetty
		_:
			pass
	# every POI: a marker pole with the name (kept from Phase 1 for readability)
	if kind != "underwater_site":
		_cyl(parent, Vector3(c.x, ground, c.z), 0.3, 6.0, Color(0.9, 0.75, 0.3))
