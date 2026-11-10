# EFTerrain — procedural archipelago terrain from the content "terrain" block (presentation only).
# Heightmap grid (CELL metres), ArrayMesh with vertex colours, HeightMapShape3D collision,
# flattened pads around POIs and straight road corridors between connected POIs.
class_name EFTerrain
extends StaticBody3D

const CELL := 16.0
const MIN_X := -2000.0
const MAX_X := 3200.0
const MIN_Z := -2400.0     # godot z = -core y
const MAX_Z := 1600.0
const POI_PAD_HEIGHT := 3.0
const PORT_PAD_HEIGHT := 1.5

var width := 0
var depth := 0
var heights := PackedFloat32Array()
var sea_floor := -60.0
var _islands: Array = []
var _pads: Array = []      # [Vector2 core_xy, radius, height]
var _roads: Array = []     # [Vector2 a, Vector2 b, ha, hb]
var _noise := FastNoiseLite.new()

func build(terrain_cfg: Dictionary, pois: Array) -> void:
	sea_floor = float(terrain_cfg.get("sea_floor", -60.0))
	_islands = terrain_cfg.get("islands", [])
	_noise.noise_type = FastNoiseLite.TYPE_SIMPLEX_SMOOTH
	_noise.frequency = 1.0 / 420.0
	_noise.fractal_octaves = 4
	var poi_by_id := {}
	for p in pois:
		poi_by_id[p["id"]] = p
		var kind: String = p.get("kind", "")
		if kind == "underwater_site":
			continue
		var h := PORT_PAD_HEIGHT if kind == "port" else POI_PAD_HEIGHT
		var r := 30.0 if kind == "port" else (120.0 if kind == "airfield" else 70.0)
		_pads.append([Vector2(p["pose"]["position"][0], p["pose"]["position"][1]), r, h])
	for road in terrain_cfg.get("roads", []):
		if poi_by_id.has(road[0]) and poi_by_id.has(road[1]):
			var a: Dictionary = poi_by_id[road[0]]; var b: Dictionary = poi_by_id[road[1]]
			_roads.append([Vector2(a["pose"]["position"][0], a["pose"]["position"][1]), Vector2(b["pose"]["position"][0], b["pose"]["position"][1]),
				PORT_PAD_HEIGHT if a.get("kind") == "port" else POI_PAD_HEIGHT, PORT_PAD_HEIGHT if b.get("kind") == "port" else POI_PAD_HEIGHT])
	width = int((MAX_X - MIN_X) / CELL) + 1
	depth = int((MAX_Z - MIN_Z) / CELL) + 1
	heights.resize(width * depth)
	for zi in range(depth):
		for xi in range(width):
			var x := MIN_X + xi * CELL
			var z := MIN_Z + zi * CELL
			heights[zi * width + xi] = _height_core(x, -z)
	_build_mesh()
	_build_collision()

# ---------------------------------------------------------------- height function (core coordinates: x, y)
func _height_core(x: float, y: float) -> float:
	var h := sea_floor + _noise.get_noise_2d(x * 0.5, y * 0.5) * 6.0
	for isl in _islands:
		var c: Array = isl["center"]; var r: Array = isl["radius"]
		var dx := (x - float(c[0])) / float(r[0]); var dy := (y - float(c[1])) / float(r[1])
		var d := dx * dx + dy * dy
		if d >= 1.4:
			continue
		var f: float = clamp(1.0 - d, 0.0, 1.0)
		f = f * f * (3.0 - 2.0 * f)   # smoothstep
		var peak := float(isl["height"])
		var rough := float(isl.get("roughness", 0.5))
		var n: float = _noise.get_noise_2d(x + float(isl.get("seed", 0)) * 1000.0, y) * rough * abs(peak) * f
		var island_h: float = lerp(sea_floor, peak, f) + n
		h = max(h, island_h)
	# POI pads
	for pad in _pads:
		var dist: float = Vector2(x, y).distance_to(pad[0])
		var r: float = pad[1]
		if dist < r * 1.6:
			var t: float = clamp((r * 1.6 - dist) / (r * 0.6), 0.0, 1.0)
			t = t * t * (3.0 - 2.0 * t)
			h = lerp(h, float(pad[2]), t)
	# roads: straight corridors, graded between the pad heights
	for rd in _roads:
		var a: Vector2 = rd[0]; var b: Vector2 = rd[1]
		var ab := b - a
		var t: float = clamp((Vector2(x, y) - a).dot(ab) / max(ab.length_squared(), 1.0), 0.0, 1.0)
		var closest := a + ab * t
		var dist := Vector2(x, y).distance_to(closest)
		if dist < 26.0:
			var w: float = clamp((26.0 - dist) / 14.0, 0.0, 1.0)
			h = lerp(h, lerp(float(rd[2]), float(rd[3]), t), w)
	return h

func height_at(gx: float, gz: float) -> float:
	var fx := (gx - MIN_X) / CELL
	var fz := (gz - MIN_Z) / CELL
	var xi: int = clamp(int(floor(fx)), 0, width - 2)
	var zi: int = clamp(int(floor(fz)), 0, depth - 2)
	var tx: float = clamp(fx - xi, 0.0, 1.0)
	var tz: float = clamp(fz - zi, 0.0, 1.0)
	var h00 := heights[zi * width + xi]; var h10 := heights[zi * width + xi + 1]
	var h01 := heights[(zi + 1) * width + xi]; var h11 := heights[(zi + 1) * width + xi + 1]
	return lerp(lerp(h00, h10, tx), lerp(h01, h11, tx), tz)

func is_road(gx: float, gz: float) -> bool:
	var p := Vector2(gx, -gz)
	for rd in _roads:
		var a: Vector2 = rd[0]; var b: Vector2 = rd[1]; var ab := b - a
		var t: float = clamp((p - a).dot(ab) / max(ab.length_squared(), 1.0), 0.0, 1.0)
		if p.distance_to(a + ab * t) < 7.0:
			return true
	return false

# ---------------------------------------------------------------- mesh + collision
func _build_mesh() -> void:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	for zi in range(depth):
		for xi in range(width):
			var h := heights[zi * width + xi]
			var x := MIN_X + xi * CELL; var z := MIN_Z + zi * CELL
			var xi1: int = min(xi + 1, width - 1); var xi0: int = max(xi - 1, 0)
			var zi1: int = min(zi + 1, depth - 1); var zi0: int = max(zi - 1, 0)
			var hx: float = heights[zi * width + xi1] - heights[zi * width + xi0]
			var hz: float = heights[zi1 * width + xi] - heights[zi0 * width + xi]
			var n := Vector3(-hx, 2.0 * CELL, -hz).normalized()
			st.set_normal(n)
			st.set_color(_color_for(h, n.y, x, z))
			st.add_vertex(Vector3(x, h, z))
	for zi in range(depth - 1):
		for xi in range(width - 1):
			var i := zi * width + xi
			st.add_index(i); st.add_index(i + width); st.add_index(i + 1)
			st.add_index(i + 1); st.add_index(i + width); st.add_index(i + width + 1)
	var mesh := st.commit()
	var mi := MeshInstance3D.new()
	mi.mesh = mesh
	var mat := StandardMaterial3D.new()
	mat.vertex_color_use_as_albedo = true
	mat.roughness = 0.95
	mi.material_override = mat
	add_child(mi)

func _color_for(h: float, ny: float, x: float, z: float) -> Color:
	if is_road(x, z) and h > 0.5:
		return Color(0.22, 0.22, 0.23)
	if h < -2.0:
		return Color(0.25, 0.3, 0.25)          # sea floor
	if h < 1.2:
		return Color(0.76, 0.7, 0.5)           # beach
	if ny < 0.72:
		return Color(0.42, 0.4, 0.37)          # rock
	if h > 32.0:
		return Color(0.46, 0.5, 0.4)           # highland scrub
	return Color(0.33, 0.52, 0.28)             # grass

func _build_collision() -> void:
	var shape := HeightMapShape3D.new()
	shape.map_width = width
	shape.map_depth = depth
	shape.map_data = heights
	var col := CollisionShape3D.new()
	col.shape = shape
	col.position = Vector3(MIN_X + (width - 1) * CELL * 0.5, 0.0, MIN_Z + (depth - 1) * CELL * 0.5)
	col.scale = Vector3(CELL, 1.0, CELL)
	add_child(col)
