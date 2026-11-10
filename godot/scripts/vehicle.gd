# Vehicle — ONE presentation controller for every vehicle class (supplement §12: one entity representation,
# base physics model + layered assists). Kinematic placeholder physics (CharacterBody3D) for Phase 1-2;
# class behaviour comes from the content "handling" block, feel from the resolved realism profile.
# The core owns identity, seats, cargo, condition and persisted pose; this node only moves the mesh and
# reports the pose at 4 Hz through World.set_pose_provider.
extends CharacterBody3D

var vehicle_id := ""
var display_name := ""
var vehicle_class := "ground"
var realism_category := "wheeled"
var control_seat := "driver"
var handling: Dictionary = {}

var possessed_by: Node = null
var chase: SpringArm3D
var chase_cam: Camera3D
var cockpit_cam: Camera3D
var cockpit_view := false
var label: Label3D

# dynamic state (presentation only)
var speed := 0.0          # forward m/s (+ forward)
var vertical := 0.0       # m/s (rotorcraft/submarine)
var steer := 0.0          # -1..1 smoothed
var depth_target := 0.0   # submarine
const SEA_LEVEL := 0.0

func setup(state: Dictionary, definition: Dictionary) -> void:
	vehicle_id = state["id"]
	display_name = definition.get("display_name", state["definition_id"])
	vehicle_class = definition.get("vehicle_class", "ground")
	realism_category = definition.get("realism_category", "wheeled")
	handling = definition.get("handling", {})
	for s in definition.get("seats", []):
		if s.get("is_control_seat", false):
			control_seat = s["seat_id"]
	var size := _size_for_class()
	var col := CollisionShape3D.new(); var box := BoxShape3D.new(); box.size = size; col.shape = box; col.position.y = size.y * 0.5; add_child(col)
	var mesh := MeshInstance3D.new(); var bm := BoxMesh.new(); bm.size = size; mesh.mesh = bm; mesh.position.y = size.y * 0.5
	var mat := StandardMaterial3D.new(); mat.albedo_color = _color_for_class(); mesh.material_override = mat; add_child(mesh)
	var nose := MeshInstance3D.new(); var nb := BoxMesh.new(); nb.size = Vector3(size.x * 0.6, size.y * 0.4, 0.6); nose.mesh = nb
	nose.position = Vector3(0, size.y * 0.5, -size.z * 0.5 - 0.3); nose.material_override = mat; add_child(nose)   # -Z is forward
	label = Label3D.new(); label.text = display_name; label.position.y = size.y + 1.5; label.font_size = 40; label.pixel_size = 0.02
	label.billboard = BaseMaterial3D.BILLBOARD_ENABLED; label.no_depth_test = true; add_child(label)
	chase = SpringArm3D.new(); chase.spring_length = max(12.0, size.z * 2.5); chase.position.y = size.y + 2.0; chase.rotation_degrees.x = -18; chase.collision_mask = 0; add_child(chase)
	chase_cam = Camera3D.new(); chase.add_child(chase_cam)
	cockpit_cam = Camera3D.new(); cockpit_cam.position = Vector3(0, size.y * 0.9, -size.z * 0.25); add_child(cockpit_cam)
	sync_from_state(state)
	set_physics_process(false)

func _size_for_class() -> Vector3:
	match vehicle_class:
		"ground": return Vector3(2.4, 2.2, 6.0)
		"armored": return Vector3(3.2, 2.6, 7.0)
		"rotorcraft": return Vector3(2.6, 2.8, 10.0)
		"fixed_wing": return Vector3(12.0, 3.0, 9.0)
		"naval": return Vector3(4.0, 2.5, 12.0)
		"submarine": return Vector3(3.5, 3.5, 16.0)
	return Vector3(2, 2, 4)

func _color_for_class() -> Color:
	match vehicle_class:
		"ground": return Color(0.85, 0.6, 0.2)
		"armored": return Color(0.45, 0.5, 0.35)
		"rotorcraft": return Color(0.9, 0.9, 0.95)
		"naval": return Color(0.3, 0.5, 0.75)
		"submarine": return Color(0.2, 0.25, 0.3)
	return Color.GRAY

func sync_from_state(state: Dictionary) -> void:
	if possessed_by != null:
		return
	var p: Vector3 = World.to_godot(state["pose"]["position"])
	if vehicle_class in ["naval", "submarine"] and p.y > SEA_LEVEL - 0.01 and p.y < 0.01:
		p.y = SEA_LEVEL
	global_position = p
	rotation_degrees.y = float(state["pose"].get("rotation", [0, 0, 0])[2])
	var cond := float(state.get("condition_normalized", 1.0))
	label.text = "%s\ncond %d%%  cargo %d" % [display_name, int(cond * 100), int(state.get("cargo_units", 0))]

# ---------------------------------------------------------------- possession (presentation side only; the core already accepted EnterVehicle)
func possess(_player: Node) -> void:
	possessed_by = _player
	speed = 0.0; vertical = 0.0; steer = 0.0
	depth_target = global_position.y
	cockpit_view = false
	chase_cam.make_current()
	set_physics_process(true)

func unpossess() -> void:
	possessed_by = null
	set_physics_process(false)
	sync_from_state(World.vehicle_state(vehicle_id))

func _unhandled_input(event: InputEvent) -> void:
	if possessed_by == null:
		return
	if event.is_action_pressed("switch_camera"):
		cockpit_view = not cockpit_view
		(cockpit_cam if cockpit_view else chase_cam).make_current()

# ---------------------------------------------------------------- shared base model + layered assists
func _physics_process(delta: float) -> void:
	var prof: Dictionary = World.resolved_profile(realism_category)
	var assist := float(prof.get("steering_assist", 50)) / 100.0
	var stab := float(prof.get("auto_stabilization", 50)) / 100.0
	var inertia := float(prof.get("inertia_influence", 50)) / 100.0
	var weather_sens := float(prof.get("weather_sensitivity", 50)) / 100.0
	var throttle := Input.get_action_strength("accelerate") - Input.get_action_strength("brake")
	var steer_in := Input.get_action_strength("steer_right") - Input.get_action_strength("steer_left")
	var up_in := Input.get_action_strength("vehicle_up") - Input.get_action_strength("vehicle_down")
	var weather: String = _region_weather()
	var wind := 0.0
	if weather == "rain":
		wind = 1.5
	elif weather == "storm":
		wind = 4.0
	wind *= weather_sens
	match vehicle_class:
		"ground", "armored": _drive(delta, throttle, steer_in, assist, stab, inertia)
		"rotorcraft": _fly(delta, throttle, steer_in, up_in, assist, stab, inertia, wind)
		"naval": _sail(delta, throttle, steer_in, assist, stab, inertia, wind, false, prof)
		"submarine": _sail(delta, throttle, steer_in, assist, stab, inertia, wind, true, prof)
		_: _drive(delta, throttle, steer_in, assist, stab, inertia)
	_update_hud()

func _drive(delta: float, throttle: float, steer_in: float, assist: float, stab: float, inertia: float) -> void:
	var max_speed := float(handling.get("max_speed_mps", 20))
	var accel := float(handling.get("accel_mps2", 3.0)) * lerp(2.0, 0.8, inertia)
	var brake := float(handling.get("brake_mps2", 6.0)) * lerp(1.6, 0.9, inertia)
	var turn := deg_to_rad(float(handling.get("turn_rate_dps", 60)))
	if throttle > 0:
		speed = move_toward(speed, max_speed, accel * delta)
	elif throttle < 0:
		speed = move_toward(speed, -max_speed * 0.3, brake * delta)
	else:
		speed = move_toward(speed, 0.0, lerp(4.0, 1.0, inertia) * delta)   # rolling resistance
	steer = move_toward(steer, steer_in, lerp(2.5, 9.0, assist) * delta)      # assist = steering response
	var speed_factor := clamp(abs(speed) / max(max_speed, 1.0), 0.0, 1.0)
	var yaw_rate := turn * steer * clamp(abs(speed) / 3.0, 0.0, 1.0) * lerp(1.0, 1.0 - 0.6 * speed_factor, stab)  # stabilization reduces high-speed oversteer
	rotation.y -= yaw_rate * delta * sign(speed if abs(speed) > 0.1 else 1.0)
	var forward := -transform.basis.z
	velocity.x = forward.x * speed; velocity.z = forward.z * speed
	velocity.y = (velocity.y - 9.8 * delta) if not is_on_floor() else 0.0
	move_and_slide()

func _fly(delta: float, throttle: float, steer_in: float, up_in: float, assist: float, stab: float, inertia: float, wind: float) -> void:
	var max_speed := float(handling.get("max_speed_mps", 60))
	var climb := float(handling.get("climb_rate_mps", 6))
	var yaw := deg_to_rad(float(handling.get("yaw_rate_dps", 60)))
	var response := lerp(0.8, 4.0, assist)
	speed = move_toward(speed, throttle * max_speed, max_speed * response * 0.25 * delta * lerp(1.5, 0.5, inertia))
	vertical = move_toward(vertical, up_in * climb, climb * response * delta * lerp(1.5, 0.5, inertia))
	if abs(up_in) < 0.01:
		vertical = move_toward(vertical, 0.0, climb * stab * 2.0 * delta)   # hover hold assist
	var yaw_in := Input.get_action_strength("yaw_right") - Input.get_action_strength("yaw_left")
	steer = move_toward(steer, yaw_in + steer_in * 0.5, response * delta)
	rotation.y -= yaw * steer * delta
	var forward := -transform.basis.z
	var drift := Vector3(wind, 0, wind * 0.5) * (1.0 - stab * 0.7)
	velocity = forward * speed + Vector3(0, vertical, 0) + drift
	if global_position.y <= 0.05 and vertical <= 0.0:
		velocity.y = 0.0
		if abs(speed) < 0.5:
			speed = 0.0
	move_and_slide()
	if global_position.y < 0.0:
		global_position.y = 0.0
	rotation.x = lerp(rotation.x, -clamp(speed / max_speed, -1, 1) * 0.2 * (1.0 - stab * 0.5), 3.0 * delta)

func _sail(delta: float, throttle: float, steer_in: float, assist: float, stab: float, inertia: float, wind: float, submarine: bool, prof: Dictionary) -> void:
	var submerged := submarine and global_position.y < SEA_LEVEL - 0.5
	var max_speed := float(handling.get("max_speed_submerged_mps" if submerged else ("max_speed_surface_mps" if submarine else "max_speed_mps"), 10))
	var accel := float(handling.get("accel_mps2", 1.0)) * lerp(2.0, 0.6, inertia)
	var turn := deg_to_rad(float(handling.get("turn_rate_dps", 30)))
	speed = move_toward(speed, throttle * max_speed, accel * delta)
	if abs(throttle) < 0.01:
		speed = move_toward(speed, 0.0, accel * 0.3 * delta)
	steer = move_toward(steer, steer_in, lerp(0.6, 3.0, assist) * delta)
	rotation.y -= turn * steer * clamp(abs(speed) / 2.0, 0.0, 1.0) * delta
	var forward := -transform.basis.z
	var wave := Vector3(wind * 0.3, 0, wind * 0.2) * float(prof.get("wave_influence", 50)) / 100.0
	velocity = forward * speed + wave
	if submarine:
		var depth_assist := float(prof.get("depth_hold_assist", 60)) / 100.0
		var dive_rate := float(handling.get("dive_rate_mps", 1.0))
		var max_depth := float(handling.get("max_depth_m", 100))
		var up_in := Input.get_action_strength("vehicle_up") - Input.get_action_strength("vehicle_down")
		if abs(up_in) > 0.01:
			depth_target = clamp(depth_target + up_in * dive_rate * delta * 2.0, -max_depth, SEA_LEVEL)
		var error := depth_target - global_position.y
		vertical = move_toward(vertical, clamp(error * lerp(0.3, 2.0, depth_assist), -dive_rate, dive_rate), dive_rate * lerp(0.5, 3.0, depth_assist) * delta)
		velocity.y = vertical
	else:
		velocity.y = 0.0
		global_position.y = SEA_LEVEL
	move_and_slide()
	if global_position.y > SEA_LEVEL:
		global_position.y = SEA_LEVEL
	rotation.z = lerp(rotation.z, -steer * 0.08 * (1.0 - stab * 0.6), 2.0 * delta)   # heel

func _region_weather() -> String:
	for r in World.state.get("regions", []):
		return str(r.get("weather", "clear"))
	return "clear"

func _update_hud() -> void:
	var hud = get_tree().current_scene.hud
	var extra := ""
	if vehicle_class == "rotorcraft":
		extra = "  alt %.0f m  vs %.1f m/s" % [global_position.y, vertical]
	elif vehicle_class == "submarine":
		extra = "  depth %.0f m (target %.0f)" % [-global_position.y, -depth_target]
	hud.set_vehicle_readout("%s · %s · %.0f km/h%s" % [display_name, World.realism.get("resolved", {}).get(realism_category, {}).get("preset_label", "?"), abs(speed) * 3.6, extra])
	var poi := get_tree().current_scene.nearest_poi_id(global_position, 40.0)
	var lines := ["[X] Exit · [C] Camera"]
	if vehicle_class == "rotorcraft":
		lines.append("[W/S] Thrust · [A/D] Bank · [Space/Ctrl] Collective · [Q/E] Yaw")
	elif vehicle_class == "submarine":
		lines.append("[W/S] Throttle · [A/D] Rudder · [Space] Surface · [Ctrl] Dive")
	else:
		lines.append("[W/S] Throttle · [A/D] Steer")
	if poi != "" and World.economy_node_at_poi(poi) != "":
		lines.append("[1-4] Load item · [R] Deliver carried")
	hud.set_prompt("\n".join(lines))
