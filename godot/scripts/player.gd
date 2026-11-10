# Player — FPS/TPS CharacterBody3D. Movement feel is modulated by the resolved "infantry" realism profile
# (supplement §10): inertia_influence smooths acceleration, fatigue_rate drains stamina while sprinting.
# No world rules here: interactions are sent to the core as commands via Game/World.
class_name EFPlayer
extends CharacterBody3D

var game: EFGame
var fps_cam: Camera3D
var tps_cam: Camera3D
var spring: SpringArm3D
var head: Node3D
var first_person := true
var yaw := 0.0
var pitch := 0.0
var stamina := 1.0
const MOUSE_SENS := 0.0025
const WALK := 4.0
const SPRINT := 7.5
const JUMP := 4.5

func _ready() -> void:
	var col := CollisionShape3D.new(); var cap := CapsuleShape3D.new(); cap.radius = 0.35; cap.height = 1.8; col.shape = cap; col.position.y = 0.9; add_child(col)
	var mesh := MeshInstance3D.new(); var cm := CapsuleMesh.new(); cm.radius = 0.35; cm.height = 1.8; mesh.mesh = cm; mesh.position.y = 0.9
	var mat := StandardMaterial3D.new(); mat.albedo_color = Color(0.25, 0.45, 0.8); mesh.material_override = mat; add_child(mesh)
	head = Node3D.new(); head.position.y = 1.6; add_child(head)
	fps_cam = Camera3D.new(); fps_cam.near = 0.05; head.add_child(fps_cam)
	spring = SpringArm3D.new(); spring.spring_length = 4.5; spring.position.y = 0.4; spring.collision_mask = 1; head.add_child(spring)
	tps_cam = Camera3D.new(); spring.add_child(tps_cam)
	fps_cam.current = true

func make_current() -> void:
	(fps_cam if first_person else tps_cam).make_current()

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		yaw -= event.relative.x * MOUSE_SENS
		pitch = clamp(pitch - event.relative.y * MOUSE_SENS, -1.3, 1.3)
		rotation.y = yaw
		head.rotation.x = pitch
	elif event.is_action_pressed("switch_camera"):
		first_person = not first_person
		make_current()          # camera is presentation only: no command, no state change (CORE-02)
	elif event.is_action_pressed("interact"):
		var npc: Node3D = game.nearest_npc(global_position, 4.0)
		if npc != null:
			game.try_interact_npc(npc)
	elif event.is_action_pressed("accept"):
		game.hud.accept_offered_mission()

func _physics_process(delta: float) -> void:
	var profile: Dictionary = World.resolved_profile("infantry")
	var inertia := float(profile.get("inertia_influence", 45)) / 100.0
	var fatigue := float(profile.get("fatigue_rate", 50)) / 100.0
	var input := Vector2(Input.get_action_strength("move_right") - Input.get_action_strength("move_left"),
						 Input.get_action_strength("move_back") - Input.get_action_strength("move_forward"))
	var dir: Vector3 = (transform.basis * Vector3(input.x, 0, input.y)).normalized() if input.length() > 0.01 else Vector3.ZERO
	var sprinting := Input.is_action_pressed("sprint") and stamina > 0.05 and dir != Vector3.ZERO
	var target_speed: float = SPRINT if sprinting else WALK
	stamina = clamp(stamina - delta * 0.25 * fatigue if sprinting else stamina + delta * 0.15, 0.0, 1.0)
	var accel: float = lerp(40.0, 6.0, inertia)       # arcade: instant; realistic: weighty
	var target := dir * target_speed
	velocity.x = move_toward(velocity.x, target.x, accel * delta)
	velocity.z = move_toward(velocity.z, target.z, accel * delta)
	if not is_on_floor():
		velocity.y -= 9.8 * delta
	elif Input.is_action_just_pressed("jump"):
		velocity.y = JUMP
	move_and_slide()
	if global_position.y < -0.5:
		global_position.y = -0.5           # placeholder: no swimming yet, keep head above water
		velocity.y = max(velocity.y, 0.0)
	_update_prompt()

func _update_prompt() -> void:
	var npc: Node3D = game.nearest_npc(global_position, 4.0)
	var veh: EFVehicle = game.nearest_vehicle(global_position, 8.0)
	var poi: String = game.nearest_poi_id(global_position, 40.0)
	var lines := []
	if npc != null:
		lines.append(EFText.t("talk_to") % npc.get_meta("npc_name"))
	if veh != null:
		lines.append(EFText.t("board") % veh.display_name)
	if poi != "" and World.economy_node_at_poi(poi) != "":
		lines.append(game.cargo_prompt())
	if veh != null and poi != "" and World.poi(poi).get("kind", "") in ["depot", "airfield", "port", "town"]:
		lines.append(game.service_prompt())
	game.hud.set_prompt("\n".join(lines))
	game.hud.set_stamina(stamina)
