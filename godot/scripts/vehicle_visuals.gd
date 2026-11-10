# VehicleVisuals — builds the visual body of a vehicle. Two paths:
#   1. godot/assets/vehicles/<definition_id>.glb exists  -> load it (real model, see Docs/ASSET_PIPELINE.md)
#   2. otherwise                                          -> procedural fictional silhouette for the class
# Returns a Dictionary with "root" (Node3D), "size" (Vector3 AABB), "spinners" (Array[Node3D] to rotate: rotors/props),
# "wheels" (Array[Node3D]). Forward is -Z (Godot convention). All designs are original/fictional by construction.
class_name VehicleVisuals

static func build(definition_id: String, vehicle_class: String, parent: Node3D) -> Dictionary:
	var glb_path := "res://assets/vehicles/%s.glb" % definition_id
	if ResourceLoader.exists(glb_path):
		var scene: PackedScene = load(glb_path)
		if scene != null:
			var inst: Node3D = scene.instantiate()
			parent.add_child(inst)
			var aabb := _aabb_of(inst)
			return {"root": inst, "size": aabb.size if aabb.size.length() > 0.1 else _size_for_class(vehicle_class), "spinners": _find_named(inst, "spin"), "wheels": _find_named(inst, "wheel"), "source": "glb"}
	var root := Node3D.new(); parent.add_child(root)
	var out := {"root": root, "size": _size_for_class(vehicle_class), "spinners": [], "wheels": [], "source": "procedural"}
	match vehicle_class:
		"ground": _truck(root, out)
		"armored": _tracked(root, out)
		"rotorcraft": _helicopter(root, out)
		"fixed_wing": _plane(root, out)
		"naval": _boat(root, out)
		"submarine": _submarine(root, out)
		_: _part(root, BoxMesh.new(), Vector3(2, 2, 4), Vector3(0, 1, 0), Vector3.ZERO, Color.GRAY)
	return out

static func _size_for_class(c: String) -> Vector3:
	match c:
		"ground": return Vector3(2.4, 2.6, 6.2)
		"armored": return Vector3(3.2, 2.6, 7.0)
		"rotorcraft": return Vector3(2.4, 3.6, 11.0)
		"fixed_wing": return Vector3(11.0, 3.0, 8.0)
		"naval": return Vector3(3.6, 3.4, 12.0)
		"submarine": return Vector3(3.4, 4.0, 16.0)
	return Vector3(2, 2, 4)

# ---------------------------------------------------------------- primitives
static func _mat(color: Color, metallic := 0.2, rough := 0.7) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = color
	m.metallic = metallic
	m.roughness = rough
	return m

static func _part(parent: Node3D, mesh: Mesh, size: Vector3, pos: Vector3, rot_deg: Vector3, color: Color, metallic := 0.2) -> MeshInstance3D:
	if mesh is BoxMesh:
		var bm: BoxMesh = mesh
		bm.size = size
	elif mesh is CylinderMesh:
		var cy: CylinderMesh = mesh
		cy.top_radius = size.x * 0.5
		cy.bottom_radius = size.z * 0.5
		cy.height = size.y
	elif mesh is CapsuleMesh:
		var ca: CapsuleMesh = mesh
		ca.radius = size.x * 0.5
		ca.height = size.y
	elif mesh is SphereMesh:
		var sp: SphereMesh = mesh
		sp.radius = size.x * 0.5
		sp.height = size.y
	elif mesh is PrismMesh:
		var pr: PrismMesh = mesh
		pr.size = size
	var mi := MeshInstance3D.new()
	mi.mesh = mesh
	mi.position = pos
	mi.rotation_degrees = rot_deg
	mi.material_override = _mat(color, metallic)
	parent.add_child(mi)
	return mi

static func _wheel(parent: Node3D, pos: Vector3, radius: float, width: float, out: Dictionary) -> void:
	var w: MeshInstance3D = _part(parent, CylinderMesh.new(), Vector3(radius * 2, width, radius * 2), pos, Vector3(0, 0, 90), Color(0.08, 0.08, 0.08), 0.0)
	out["wheels"].append(w)

# ---------------------------------------------------------------- fictional designs
const PAINT_UTILITY := Color(0.78, 0.55, 0.18)
const PAINT_ARMOR := Color(0.38, 0.43, 0.30)
const PAINT_RESCUE := Color(0.92, 0.92, 0.95)
const PAINT_NAVY := Color(0.42, 0.47, 0.52)
const PAINT_SUB := Color(0.15, 0.17, 0.20)
const GLASS := Color(0.35, 0.55, 0.7, 0.85)

static func _truck(r: Node3D, out: Dictionary) -> void:
	_part(r, BoxMesh.new(), Vector3(2.3, 0.4, 6.0), Vector3(0, 0.8, 0), Vector3.ZERO, Color(0.2, 0.2, 0.22))          # chassis
	_part(r, BoxMesh.new(), Vector3(2.2, 1.6, 1.9), Vector3(0, 1.8, -1.9), Vector3.ZERO, PAINT_UTILITY)                 # cab
	_part(r, BoxMesh.new(), Vector3(2.0, 0.8, 0.1), Vector3(0, 2.1, -2.86), Vector3.ZERO, GLASS, 0.6)                   # windscreen
	_part(r, BoxMesh.new(), Vector3(2.2, 0.9, 0.8), Vector3(0, 1.45, -3.2), Vector3.ZERO, PAINT_UTILITY)                # hood
	_part(r, BoxMesh.new(), Vector3(2.3, 1.1, 3.4), Vector3(0, 1.55, 0.9), Vector3.ZERO, Color(0.55, 0.45, 0.3))        # cargo bed
	_part(r, BoxMesh.new(), Vector3(2.3, 0.15, 3.4), Vector3(0, 2.15, 0.9), Vector3.ZERO, Color(0.35, 0.4, 0.3))        # tarp
	for z in [-2.0, 0.6, 1.9]:
		_wheel(r, Vector3(-1.2, 0.5, z), 0.5, 0.35, out); _wheel(r, Vector3(1.2, 0.5, z), 0.5, 0.35, out)

static func _tracked(r: Node3D, out: Dictionary) -> void:
	_part(r, BoxMesh.new(), Vector3(2.4, 1.1, 6.4), Vector3(0, 1.2, 0), Vector3.ZERO, PAINT_ARMOR)                       # hull
	_part(r, PrismMesh.new(), Vector3(2.4, 0.9, 1.6), Vector3(0, 1.95, -2.2), Vector3(0, 0, 0), PAINT_ARMOR)             # glacis
	for x in [-1.45, 1.45]:
		_part(r, BoxMesh.new(), Vector3(0.7, 0.9, 6.8), Vector3(x, 0.55, 0), Vector3.ZERO, Color(0.1, 0.1, 0.1))         # tracks
		for z in [-2.6, -1.3, 0.0, 1.3, 2.6]:
			out["wheels"].append(_part(r, CylinderMesh.new(), Vector3(0.7, 0.75, 0.7), Vector3(x, 0.5, z), Vector3(0, 0, 90), Color(0.25, 0.25, 0.25)))
	_part(r, CylinderMesh.new(), Vector3(2.0, 0.7, 2.2), Vector3(0, 2.1, 0.4), Vector3.ZERO, PAINT_ARMOR)                # low turret (fictional)
	_part(r, BoxMesh.new(), Vector3(0.6, 0.5, 1.4), Vector3(0, 2.6, 0.4), Vector3.ZERO, Color(0.3, 0.33, 0.25))          # sensor mast
	_part(r, BoxMesh.new(), Vector3(0.9, 0.6, 1.2), Vector3(0.6, 2.3, 1.6), Vector3.ZERO, PAINT_ARMOR)                   # commander hatch block

static func _helicopter(r: Node3D, out: Dictionary) -> void:
	_part(r, CapsuleMesh.new(), Vector3(2.2, 5.2, 2.2), Vector3(0, 1.9, -0.6), Vector3(90, 0, 0), PAINT_RESCUE)          # cabin
	_part(r, SphereMesh.new(), Vector3(1.9, 1.9, 1.9), Vector3(0, 1.9, -3.1), Vector3.ZERO, GLASS, 0.6)                  # canopy
	_part(r, CylinderMesh.new(), Vector3(0.6, 5.5, 0.35), Vector3(0, 2.2, 4.2), Vector3(90, 0, 0), PAINT_RESCUE)          # tail boom
	_part(r, BoxMesh.new(), Vector3(0.15, 1.6, 0.9), Vector3(0, 3.1, 6.6), Vector3(0, 0, 0), Color(0.85, 0.2, 0.2))       # fin (red stripe)
	_part(r, BoxMesh.new(), Vector3(1.8, 0.1, 0.5), Vector3(0, 2.6, 6.2), Vector3.ZERO, PAINT_RESCUE)                     # stabiliser
	var tail := _part(r, BoxMesh.new(), Vector3(0.08, 1.6, 0.18), Vector3(0.35, 3.1, 6.9), Vector3.ZERO, Color(0.1, 0.1, 0.1)); out["spinners"].append(tail)
	_part(r, CylinderMesh.new(), Vector3(0.3, 0.6, 0.3), Vector3(0, 3.3, -0.4), Vector3.ZERO, Color(0.1, 0.1, 0.1))       # mast
	var rotor := Node3D.new(); rotor.position = Vector3(0, 3.65, -0.4); r.add_child(rotor)
	_part(rotor, BoxMesh.new(), Vector3(11.0, 0.06, 0.4), Vector3.ZERO, Vector3.ZERO, Color(0.08, 0.08, 0.08))
	_part(rotor, BoxMesh.new(), Vector3(0.4, 0.06, 11.0), Vector3.ZERO, Vector3.ZERO, Color(0.08, 0.08, 0.08))
	out["spinners"].append(rotor)
	for x in [-1.0, 1.0]:
		_part(r, CylinderMesh.new(), Vector3(0.12, 3.6, 0.12), Vector3(x, 0.15, -0.5), Vector3(90, 0, 0), Color(0.2, 0.2, 0.2))   # skids
		_part(r, CylinderMesh.new(), Vector3(0.1, 0.9, 0.1), Vector3(x, 0.6, -1.5), Vector3.ZERO, Color(0.2, 0.2, 0.2))
		_part(r, CylinderMesh.new(), Vector3(0.1, 0.9, 0.1), Vector3(x, 0.6, 0.6), Vector3.ZERO, Color(0.2, 0.2, 0.2))

static func _plane(r: Node3D, out: Dictionary) -> void:
	_part(r, CapsuleMesh.new(), Vector3(1.3, 7.4, 1.3), Vector3(0, 1.5, 0), Vector3(90, 0, 0), Color(0.9, 0.85, 0.3))      # fuselage (fictional trainer livery)
	_part(r, BoxMesh.new(), Vector3(11.0, 0.18, 1.6), Vector3(0, 1.95, -0.6), Vector3.ZERO, Color(0.9, 0.85, 0.3))       # high wing
	_part(r, BoxMesh.new(), Vector3(1.0, 0.7, 1.4), Vector3(0, 2.1, -1.0), Vector3.ZERO, GLASS, 0.6)                     # canopy
	_part(r, BoxMesh.new(), Vector3(3.6, 0.12, 1.0), Vector3(0, 1.6, 3.4), Vector3.ZERO, Color(0.9, 0.85, 0.3))          # tailplane
	_part(r, BoxMesh.new(), Vector3(0.12, 1.5, 1.1), Vector3(0, 2.3, 3.5), Vector3.ZERO, Color(0.85, 0.2, 0.2))          # fin
	var prop := Node3D.new(); prop.position = Vector3(0, 1.5, -3.9); r.add_child(prop)
	_part(prop, BoxMesh.new(), Vector3(2.2, 0.15, 0.06), Vector3.ZERO, Vector3.ZERO, Color(0.1, 0.1, 0.1))
	_part(prop, BoxMesh.new(), Vector3(0.15, 2.2, 0.06), Vector3.ZERO, Vector3.ZERO, Color(0.1, 0.1, 0.1))
	out["spinners"].append(prop)
	_wheel(r, Vector3(0, 0.35, -2.4), 0.35, 0.2, out); _wheel(r, Vector3(-1.4, 0.35, 0.4), 0.35, 0.2, out); _wheel(r, Vector3(1.4, 0.35, 0.4), 0.35, 0.2, out)

static func _boat(r: Node3D, out: Dictionary) -> void:
	_part(r, BoxMesh.new(), Vector3(3.4, 1.4, 9.0), Vector3(0, 0.5, 1.0), Vector3.ZERO, PAINT_NAVY)                     # hull
	_part(r, PrismMesh.new(), Vector3(3.4, 1.4, 3.0), Vector3(0, 0.5, -5.0), Vector3(0, 0, 0), PAINT_NAVY)                # bow
	_part(r, BoxMesh.new(), Vector3(3.6, 0.2, 12.0), Vector3(0, 1.25, 0.0), Vector3.ZERO, Color(0.55, 0.5, 0.42))        # deck
	_part(r, BoxMesh.new(), Vector3(2.4, 1.6, 2.6), Vector3(0, 2.1, -0.6), Vector3.ZERO, Color(0.9, 0.9, 0.9))           # wheelhouse
	_part(r, BoxMesh.new(), Vector3(2.2, 0.7, 0.1), Vector3(0, 2.4, -1.95), Vector3.ZERO, GLASS, 0.6)
	_part(r, CylinderMesh.new(), Vector3(0.12, 2.6, 0.12), Vector3(0, 3.8, -0.2), Vector3.ZERO, Color(0.3, 0.3, 0.3))    # mast
	var radar := _part(r, BoxMesh.new(), Vector3(1.4, 0.15, 0.2), Vector3(0, 4.3, -0.2), Vector3.ZERO, Color(0.3, 0.3, 0.3)); out["spinners"].append(radar)
	_part(r, BoxMesh.new(), Vector3(0.5, 0.6, 1.2), Vector3(0, 1.6, 4.8), Vector3.ZERO, Color(0.9, 0.5, 0.1))            # stern locker

static func _submarine(r: Node3D, out: Dictionary) -> void:
	_part(r, CapsuleMesh.new(), Vector3(3.2, 15.0, 3.2), Vector3(0, 0.0, 0), Vector3(90, 0, 0), PAINT_SUB, 0.4)          # pressure hull (waterline at y=0)
	_part(r, BoxMesh.new(), Vector3(1.2, 2.2, 3.0), Vector3(0, 2.4, -1.5), Vector3.ZERO, PAINT_SUB, 0.4)                 # sail
	_part(r, BoxMesh.new(), Vector3(3.6, 0.15, 1.0), Vector3(0, 2.6, -1.5), Vector3.ZERO, PAINT_SUB)                     # sail planes
	_part(r, BoxMesh.new(), Vector3(4.0, 0.15, 1.2), Vector3(0, 0.0, 6.6), Vector3.ZERO, PAINT_SUB)                      # stern planes
	_part(r, BoxMesh.new(), Vector3(0.15, 3.2, 1.2), Vector3(0, 0.0, 6.6), Vector3.ZERO, PAINT_SUB)                      # rudder
	var prop := Node3D.new(); prop.position = Vector3(0, 0, 7.8); r.add_child(prop)
	_part(prop, BoxMesh.new(), Vector3(2.0, 0.3, 0.08), Vector3.ZERO, Vector3.ZERO, Color(0.6, 0.45, 0.2), 0.8)
	_part(prop, BoxMesh.new(), Vector3(0.3, 2.0, 0.08), Vector3.ZERO, Vector3.ZERO, Color(0.6, 0.45, 0.2), 0.8)
	out["spinners"].append(prop)
	_part(r, BoxMesh.new(), Vector3(0.3, 0.6, 0.3), Vector3(0.3, 3.7, -2.2), Vector3.ZERO, Color(0.3, 0.3, 0.3))          # mast

# ---------------------------------------------------------------- glb helpers
static func _aabb_of(n: Node) -> AABB:
	var box := AABB()
	var first := true
	for c in n.find_children("*", "MeshInstance3D", true, false):
		var mi: MeshInstance3D = c
		var a: AABB = mi.get_aabb()
		a.position += mi.position
		if first:
			box = a
			first = false
		else:
			box = box.merge(a)
	return box

static func _find_named(n: Node, needle: String) -> Array:
	var res := []
	for c in n.find_children("*" + needle + "*", "Node3D", true, false):
		res.append(c)
	return res
