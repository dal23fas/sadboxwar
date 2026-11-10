# Vehicle — ONE presentation controller for every vehicle class (supplement §12: one entity representation,
# base physics model + layered assists). Kinematic placeholder physics (CharacterBody3D) for Phase 1-2;
# class behaviour comes from the content "handling" block, feel from the resolved realism profile.
# The core owns identity, seats, cargo, condition and persisted pose; this node only moves the mesh and
# reports the pose at 4 Hz through World.set_pose_provider.
class_name EFVehicle
extends CharacterBody3D

var vehicle_id := ""
var display_name := ""
var vehicle_class := "ground"
var realism_category := "wheeled"
var control_seat := "driver"
var handling: Dictionary = {}
var terrain: EFTerrain = null
var fuel := 1.0
var condition := 1.0

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
var spinners: Array = []
var wheels: Array = []
var throttle_setting := 0.0   # fixed-wing persistent throttle 0..1
var pitch_angle := 0.0        # fixed-wing
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
	var visuals := VehicleVisuals.build(state["definition_id"], vehicle_class, self)
	var size: Vector3 = visuals["size"]
	spinners = visuals["spinners"]
	wheels = visuals["wheels"]
	var col := CollisionShape3D.new(); var box := BoxShape3D.new(); box.size = size; col.shape = box; col.position.y = size.y * 0.5; add_child(col)
	label = Label3D.new(); label.text = display_name; label.position.y = size.y + 1.5; label.font_size = 40; label.pixel_size = 0.02
	label.billboard = BaseMaterial3D.BILLBOARD_ENABLED; label.no_depth_test = true; add_child(label)
	chase = SpringArm3D.new(); chase.spring_length = max(12.0, size.z * 2.5); chase.position.y = size.y + 2.0; chase.rotation_degrees.x = -18; chase.collision_mask = 0; add_child(chase)
	chase_cam = Camera3D.new(); chase.add_child(chase_cam)
	cockpit_cam = Camera3D.new(); cockpit_cam.position = Vector3(0, size.y * 0.9, -size.z * 0.25); add_child(cockpit_cam)
	sync_from_state(state)
	set_physics_process(false)

func sync_from_state(state: Dictionary) -> void:
	if possessed_by != null:
		return
	var p: Vector3 = World.to_godot(state["pose"]["position"])
	if vehicle_class in ["naval", "submarine"]:
		if p.y > SEA_LEVEL - 0.01:
			p.y = SEA_LEVEL
	elif terrain != null:
		p.y = max(p.y, terrain.height_at(p.x, p.z) + 0.05)
	global_position = p
	fuel = float(state.get("fuel_normalized", 1.0))
	condition = float(state.get("condition_normalized", 1.0))
	rotation_degrees.y = float(state["pose"].get("rotation", [0, 0, 0])[2])
	label.text = "%s\ncond %d%%  fuel %d%%  cargo %d" % [display_name, int(condition * 100), int(fuel * 100), int(state.get("cargo_units", 0))]

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
	rotation.x = 0.0
	rotation.z = 0.0
	pitch_angle = 0.0
	throttle_setting = 0.0
	sync_from_state(World.vehicle_state(vehicle_id))

func _unhandled_input(event: InputEvent) -> void:
	if possessed_by == null:
		return
	if event.is_action_pressed("switch_camera"):
		cockpit_view = not cockpit_view
		(cockpit_cam if cockpit_view else chase_cam).make_current()

# ---------------------------------------------------------------- shared base model + layered assists
func _physics_process(delta: float) -> void:
	var vs := World.vehicle_state(vehicle_id)
	fuel = float(vs.get("fuel_normalized", fuel))
	condition = float(vs.get("condition_normalized", condition))
	var prof: Dictionary = World.resolved_profile(realism_category)
	var assist := float(prof.get("steering_assist", 50)) / 100.0
	var stab := float(prof.get("auto_stabilization", 50)) / 100.0
	var inertia := float(prof.get("inertia_influence", 50)) / 100.0
	var weather_sens := float(prof.get("weather_sensitivity", 50)) / 100.0
	var throttle := Input.get_action_strength("accelerate") - Input.get_action_strength("brake")
	if fuel <= 0.001:
		throttle = min(throttle, 0.0)       # out of fuel: coast / brake only
	elif condition < 0.25:
		throttle *= 0.4                     # badly damaged: limp
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
		"fixed_wing": _fly_plane(delta, throttle, steer_in, up_in, assist, stab, inertia, wind)
		"naval": _sail(delta, throttle, steer_in, assist, stab, inertia, wind, false, prof)
		"submarine": _sail(delta, throttle, steer_in, assist, stab, inertia, wind, true, prof)
		_: _drive(delta, throttle, steer_in, assist, stab, inertia)
	_update_hud()

func _drive(delta: float, throttle: float, steer_in: float, assist: float, stab: float, inertia: float) -> void:
	var max_speed := float(handling.get("max_speed_mps", 20))
	var accel: float = float(handling.get("accel_mps2", 3.0)) * lerp(2.0, 0.8, inertia)
	var brake: float = float(handling.get("brake_mps2", 6.0)) * lerp(1.6, 0.9, inertia)
	var turn := deg_to_rad(float(handling.get("turn_rate_dps", 60)))
	if throttle > 0:
		speed = move_toward(speed, max_speed, accel * delta)
	elif throttle < 0:
		speed = move_toward(speed, -max_speed * 0.3, brake * delta)
	else:
		speed = move_toward(speed, 0.0, lerp(4.0, 1.0, inertia) * delta)   # rolling resistance
	steer = move_toward(steer, steer_in, lerp(2.5, 9.0, assist) * delta)      # assist = steering response
	var speed_factor: float = clamp(abs(speed) / max(max_speed, 1.0), 0.0, 1.0)
	var pivot: float = 1.0 if vehicle_class == "armored" else clamp(abs(speed) / 3.0, 0.0, 1.0)   # tracked vehicles pivot in place
	var yaw_rate: float = turn * steer * pivot * lerp(1.0, 1.0 - 0.6 * speed_factor, stab)  # stabilization reduces high-speed oversteer
	rotation.y -= yaw_rate * delta * sign(speed if abs(speed) > 0.1 else 1.0)
	var forward := -transform.basis.z
	velocity.x = forward.x * speed; velocity.z = forward.z * speed
	velocity.y = (velocity.y - 9.8 * delta) if not is_on_floor() else 0.0
	move_and_slide()
	if terrain != null:
		var g := terrain.height_at(global_position.x, global_position.z)
		if global_position.y < g - 0.5:
			global_position.y = g + 0.05
	# damage from hard collisions (abstract, non-graphic): only on real impacts at speed
	if get_slide_collision_count() > 0 and abs(speed) > 8.0 and condition > 0.0:
		var n: Vector3 = get_slide_collision(0).get_normal()
		if abs(n.y) < 0.5:
			speed *= 0.3
			World.apply({"type": "SetVehicleCondition", "vehicle_id": vehicle_id, "condition_normalized": condition - 0.05, "reason_tag": "collision"})

func _fly(delta: float, throttle: float, steer_in: float, up_in: float, assist: float, stab: float, inertia: float, wind: float) -> void:
	var max_speed := float(handling.get("max_speed_mps", 60))
	var climb := float(handling.get("climb_rate_mps", 6))
	var yaw := deg_to_rad(float(handling.get("yaw_rate_dps", 60)))
	var response: float = lerp(0.8, 4.0, assist)
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
	var gh: float = terrain.height_at(global_position.x, global_position.z) if terrain != null else 0.0
	if global_position.y <= gh + 0.05 and vertical <= 0.0:
		velocity.y = 0.0
		if abs(speed) < 0.5:
			speed = 0.0
	move_and_slide()
	var ground_h: float = terrain.height_at(global_position.x, global_position.z) if terrain != null else 0.0
	if global_position.y < ground_h:
		global_position.y = ground_h
		vertical = 0.0
	rotation.x = lerp(rotation.x, -clamp(speed / max_speed, -1, 1) * 0.2 * (1.0 - stab * 0.5), 3.0 * delta)

func _sail(delta: float, throttle: float, steer_in: float, assist: float, stab: float, inertia: float, wind: float, submarine: bool, prof: Dictionary) -> void:
	var submerged := submarine and global_position.y < SEA_LEVEL - 0.5
	var max_speed: float = float(handling.get("max_speed_submerged_mps" if submerged else ("max_speed_surface_mps" if submarine else "max_speed_mps"), 10))
	var accel: float = float(handling.get("accel_mps2", 1.0)) * lerp(2.0, 0.6, inertia)
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
	if submarine and terrain != null:
		var floor_h: float = terrain.height_at(global_position.x, global_position.z) + 2.5
		if global_position.y < floor_h:
			global_position.y = floor_h
			if depth_target < floor_h:
				depth_target = floor_h
	rotation.z = lerp(rotation.z, -steer * 0.08 * (1.0 - stab * 0.6), 2.0 * delta)   # heel

func _region_weather() -> String:
	for r in World.state.get("regions", []):
		return str(r.get("weather", "clear"))
	return "clear"

func _update_hud() -> void:
	var game: EFGame = get_tree().current_scene as EFGame
	var hud: EFHud = game.hud
	var extra := ""
	if vehicle_class == "rotorcraft":
		extra = "  alt %.0f m  vs %.1f m/s" % [global_position.y, vertical]
	elif vehicle_class == "submarine":
		extra = "  depth %.0f m (target %.0f)" % [-global_position.y, -depth_target]
	elif vehicle_class == "fixed_wing":
		extra = "  alt %.0f m  throttle %d%%  %s" % [global_position.y, int(throttle_setting * 100), "STALL" if speed < float(handling.get("stall_speed_mps", 25)) and global_position.y > 1.0 else ""]
	hud.set_vehicle_readout("%s · %s · %.0f km/h · fuel %d%% · cond %d%%%s" % [display_name, World.realism.get("resolved", {}).get(realism_category, {}).get("preset_label", "?"), abs(speed) * 3.6, int(fuel * 100), int(condition * 100), extra])
	var poi: String = game.nearest_poi_id(global_position, 40.0)
	var lines := ["[X] Exit · [C] Camera"]
	if vehicle_class == "rotorcraft":
		lines.append("[W/S] Thrust · [A/D] Bank · [Space/Ctrl] Collective · [Q/E] Yaw")
	elif vehicle_class == "submarine":
		lines.append("[W/S] Throttle · [A/D] Rudder · [Space] Surface · [Ctrl] Dive")
	elif vehicle_class == "fixed_wing":
		lines.append("[W/S] Throttle up/down · [A/D] Roll · [Space] Pitch up · [Ctrl] Pitch down")
	else:
		lines.append("[W/S] Throttle · [A/D] Steer")
	if poi != "" and World.economy_node_at_poi(poi) != "":
		lines.append(game.cargo_prompt())
	if poi != "" and World.poi(poi).get("kind", "") in ["depot", "airfield", "port", "town"]:
		lines.append(game.service_prompt())
	hud.set_prompt("\n".join(lines))

# ---------------------------------------------------------------- fixed-wing (kinematic game physics, assists layered)
func _fly_plane(delta: float, throttle_in: float, roll_in: float, pitch_in: float, assist: float, stab: float, inertia: float, wind: float) -> void:
	var max_speed := float(handling.get("max_speed_mps", 70))
	var stall := float(handling.get("stall_speed_mps", 25))
	var climb := float(handling.get("climb_rate_mps", 5))
	var roll_rate := deg_to_rad(float(handling.get("roll_rate_dps", 90)))
	var pitch_rate := deg_to_rad(float(handling.get("pitch_rate_dps", 30)))
	throttle_setting = clamp(throttle_setting + throttle_in * delta * lerp(0.9, 0.35, inertia), 0.0, 1.0)
	var target_speed := throttle_setting * max_speed
	speed = move_toward(speed, target_speed, max_speed * 0.25 * delta * lerp(1.6, 0.6, inertia))
	var ground_h: float = terrain.height_at(global_position.x, global_position.z) if terrain != null else 0.0
	var grounded := global_position.y <= ground_h + 0.3 and vertical <= 0.0
	var flying := speed > stall
	# roll -> bank -> turn; arcade assist auto-levels the wings
	var roll_target := -roll_in * deg_to_rad(45.0)
	rotation.z = move_toward(rotation.z, roll_target if abs(roll_in) > 0.01 else lerp(rotation.z, 0.0, stab), roll_rate * delta * lerp(0.6, 1.4, assist))
	if flying:
		rotation.y -= rotation.z * 0.9 * delta * (speed / max_speed + 0.3)
	# pitch; auto-trim toward level when no input (assist)
	var pitch_target := pitch_in * deg_to_rad(25.0)
	pitch_angle = move_toward(pitch_angle, pitch_target if abs(pitch_in) > 0.01 else lerp(pitch_angle, 0.0, stab * 0.5), pitch_rate * delta * lerp(0.6, 1.5, assist))
	if grounded and pitch_angle < 0.0:
		pitch_angle = 0.0
	rotation.x = pitch_angle * 0.6
	var forward := -transform.basis.z
	var lift_factor: float = clamp((speed - stall) / max(stall, 1.0), 0.0, 1.0)
	if flying:
		vertical = sin(pitch_angle) * speed * 0.5 * lift_factor + (climb * 0.3 * lift_factor if pitch_angle > 0.05 else 0.0)
		if abs(pitch_in) < 0.01 and stab > 0.6:
			vertical = lerp(vertical, 0.0, 0.5)     # altitude hold (arcade/balanced)
	else:
		vertical = move_toward(vertical, -6.0 * (1.0 - lift_factor), 4.0 * delta)   # below stall speed: sink
		if grounded:
			vertical = 0.0
	var drift := Vector3(wind, 0, wind * 0.4) * (1.0 - stab * 0.6)
	velocity = Vector3(forward.x, 0, forward.z).normalized() * speed + Vector3(0, vertical, 0) + drift
	move_and_slide()
	if global_position.y < ground_h:
		global_position.y = ground_h
		vertical = 0.0
		if speed > stall * 1.3 and throttle_setting < 0.3:
			speed = move_toward(speed, 0.0, 8.0 * delta)   # hard landing: brakes bite (abstract)

func _process(delta: float) -> void:
	var spin := 0.0
	match vehicle_class:
		"rotorcraft": spin = (6.0 if possessed_by != null else 0.0) + abs(vertical) + abs(speed) * 0.05
		"fixed_wing": spin = throttle_setting * 40.0 + abs(speed) * 0.2
		"naval": spin = 1.5
		"submarine": spin = abs(speed) * 1.5
	for sp in spinners:
		if vehicle_class == "fixed_wing" or vehicle_class == "submarine":
			sp.rotate_z(spin * delta)
		elif vehicle_class == "naval":
			sp.rotate_y(spin * delta)
		else:
			sp.rotate_y(spin * delta)
	for w in wheels:
		w.rotate_object_local(Vector3(0, 1, 0), speed * delta * 2.0)
