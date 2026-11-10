# Game — builds the Asterion bootstrap scene from the content projection and runs the
# on-foot / in-vehicle flow. Everything here is presentation: positions are reported to the core,
# interactions become commands, the HUD reads projections.
class_name EFGame
extends Node3D

const SAVE_SLOT := "quick"

var player: EFPlayer
var hud: EFHud
var realism_panel: EFRealismPanel
var vehicles: Dictionary = {}        # vehicle_id -> Node3D
var npc_markers: Dictionary = {}     # npc_id -> Node3D
var poi_markers: Dictionary = {}     # poi_id -> Node3D
var possessed: EFVehicle = null      # vehicle node when seated
var paused := false

func _ready() -> void:
	hud = EFHud.new(); add_child(hud)
	realism_panel = EFRealismPanel.new(); hud.add_child(realism_panel)
	World.world_event.connect(_on_world_event)
	World.state_updated.connect(_on_state_updated)
	hud.show_status("Connecting to EFWorldCore…")
	var c: Dictionary = World.connect_backend()
	if not c.get("ok", false) and "--smoke" in OS.get_cmdline_user_args():
		print("SMOKE FAIL: core not available: %s" % c.get("reason", "?"))
		get_tree().quit(1)
		return
	if not c.get("ok", false):
		hud.show_fatal("CORE NOT AVAILABLE — %s\n\nStart the dev server first, then press F5 / restart the scene:\n  Windows:  build\\Release\\ef_server.exe   (or build\\ef_server.exe)\n  Linux/macOS:  ./build/ef_server\n\nBuild it once with:  cmake -S . -B build && cmake --build build --config Release\nOr build the GDExtension (Source/EFGodot) so no server is needed." % c.get("reason", "?"))
		print("[Game] core not available: ", c.get("reason", "?"))
		set_process_unhandled_input(true)
		return
	_on_world_connected(c["backend"])

func _retry_connect() -> void:
	var c: Dictionary = World.connect_backend()
	if c.get("ok", false):
		hud.hide_fatal()
		_on_world_connected(c["backend"])
	else:
		hud.toast("still no core: %s" % c.get("reason", "?"))

func _on_world_connected(backend: String) -> void:
	var content_path := "res://data/asterion_island_v1.json"      # exported builds / CI carry a copy of Content/Data here
	var presets_path := "res://data/realism_presets_v1.json"
	if not FileAccess.file_exists(content_path):
		content_path = ProjectSettings.globalize_path("res://").path_join("../Content/Data/asterion_island_v1.json")
		presets_path = ProjectSettings.globalize_path("res://").path_join("../Content/Data/realism_presets_v1.json")
	if not World.start_new_game(content_path, presets_path):
		hud.show_fatal("start_new_game failed (see Output)")
		return
	hud.show_status("")
	hud.toast("Core: %s · content %s" % [backend, World.content.get("content_id", "?")])
	_build_environment()
	_build_pois()
	_build_npcs()
	_build_vehicles()
	_spawn_player()
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	if "--smoke" in OS.get_cmdline_user_args():
		var t := EFSmokeTest.new(self)
		add_child(t)
		t.run()

# ---------------------------------------------------------------- scene construction (placeholder meshes)
func _build_environment() -> void:
	var sun := DirectionalLight3D.new(); sun.rotation_degrees = Vector3(-50, 30, 0); sun.shadow_enabled = true; add_child(sun)
	var env := WorldEnvironment.new(); var e := Environment.new()
	e.background_mode = Environment.BG_SKY; e.sky = Sky.new(); e.sky.sky_material = ProceduralSkyMaterial.new()
	e.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	env.environment = e; add_child(env)
	# land: big flat slab; sea: blue plane to the west/north where the harbour, reef and Pinewatch are
	_add_slab(Vector3(400, -0.5, 0), Vector3(3200, 1, 3200), Color(0.36, 0.45, 0.30), true)
	_add_slab(Vector3(-1400, -0.6, -300), Vector3(2000, 1, 3400), Color(0.12, 0.30, 0.48), false)
	# Windfield runway (core (400,-500) -> godot (400, 0, 500)), along x
	_add_slab(Vector3(400, -0.44, 500), Vector3(700, 0.12, 30), Color(0.3, 0.3, 0.32), false)
	# Harbour West quay and Depot South apron
	_add_slab(Vector3(-560, -0.44, -150), Vector3(80, 0.12, 60), Color(0.45, 0.42, 0.4), false)
	_add_slab(Vector3(100, -0.44, 250), Vector3(90, 0.12, 70), Color(0.3, 0.3, 0.32), false)
	# a few blocks so the town reads as a town
	for i in range(6):
		_add_slab(Vector3(-25 + i * 10, 3, 20 + (i % 2) * 14), Vector3(7, 6, 7), Color(0.72, 0.68, 0.6), true)

func _add_slab(pos: Vector3, size: Vector3, color: Color, solid: bool) -> void:
	var body := StaticBody3D.new(); body.position = pos
	var mesh := MeshInstance3D.new(); var box := BoxMesh.new(); box.size = size; mesh.mesh = box
	var mat := StandardMaterial3D.new(); mat.albedo_color = color; mesh.material_override = mat
	body.add_child(mesh)
	if solid:
		var col := CollisionShape3D.new(); var shape := BoxShape3D.new(); shape.size = size; col.shape = shape; body.add_child(col)
	add_child(body)

func _label(text: String, height: float) -> Label3D:
	var l := Label3D.new(); l.text = text; l.position.y = height; l.font_size = 48; l.pixel_size = 0.02
	l.billboard = BaseMaterial3D.BILLBOARD_ENABLED; l.no_depth_test = true
	return l

func _build_pois() -> void:
	for p in World.content.get("pois", []):
		var pos: Vector3 = World.to_godot(p["pose"]["position"])
		var m := Node3D.new(); m.position = pos; m.name = "POI_" + p["id"]
		var mesh := MeshInstance3D.new(); var cyl := CylinderMesh.new(); cyl.top_radius = 6; cyl.bottom_radius = 6; cyl.height = 0.3
		mesh.mesh = cyl; var mat := StandardMaterial3D.new(); mat.albedo_color = Color(0.9, 0.75, 0.3, 1); mesh.material_override = mat
		m.add_child(mesh); m.add_child(_label(p["name"] + "\n[" + p["kind"] + "]", 8))
		add_child(m); poi_markers[p["id"]] = m

func _build_npcs() -> void:
	for n in World.content.get("npcs", []):
		var pos: Vector3 = World.to_godot(n["pose"]["position"]) + Vector3(4, 0, 4)
		var m := Node3D.new(); m.position = pos; m.name = "NPC_" + n["id"]
		var mesh := MeshInstance3D.new(); var cap := CapsuleMesh.new(); cap.radius = 0.35; cap.height = 1.8; mesh.mesh = cap; mesh.position.y = 0.9
		var mat := StandardMaterial3D.new(); mat.albedo_color = Color(0.85, 0.35, 0.3); mesh.material_override = mat
		m.add_child(mesh); m.add_child(_label(n["name"], 2.4))
		m.set_meta("npc_id", n["id"]); m.set_meta("npc_name", n["name"])
		add_child(m); npc_markers[n["id"]] = m

func _build_vehicles() -> void:
	for v in World.state.get("vehicles", []):
		var node := EFVehicle.new(); node.name = "VEH_" + v["id"]
		add_child(node)
		node.setup(v, World.vehicle_definition(v["definition_id"]))
		vehicles[v["id"]] = node

func _spawn_player() -> void:
	player = EFPlayer.new(); player.name = "Player"
	add_child(player)
	player.position = World.to_godot(World.state["player"]["pose"]["position"]) + Vector3(0, 1.2, 6)
	player.game = self
	World.set_pose_provider(_pose_provider)
	player.make_current()

# ---------------------------------------------------------------- pose reporting (presentation -> domain, 4 Hz)
func _pose_provider():
	if possessed != null:
		return {"entity_id": possessed.vehicle_id, "position": possessed.global_position, "yaw_deg": possessed.rotation_degrees.y}
	if player != null:
		return {"entity_id": "player_main", "position": player.global_position, "yaw_deg": player.rotation_degrees.y}
	return null

# ---------------------------------------------------------------- interaction helpers used by player/vehicle
func nearest_npc(from: Vector3, radius: float) -> Node3D:
	var best: Node3D = null; var best_d := radius
	for n in npc_markers.values():
		var d := from.distance_to(n.global_position)
		if d < best_d:
			best_d = d
			best = n
	return best

func nearest_vehicle(from: Vector3, radius: float) -> EFVehicle:
	var best: EFVehicle = null; var best_d := radius
	for v in vehicles.values():
		var d := from.distance_to(v.global_position)
		if d < best_d:
			best_d = d
			best = v
	return best

func nearest_poi_id(from: Vector3, radius: float) -> String:
	var best := ""; var best_d := radius
	for id in poi_markers.keys():
		var d := from.distance_to(poi_markers[id].global_position)
		if d < best_d:
			best_d = d
			best = id
	return best

func try_interact_npc(npc_node: Node3D) -> void:
	var npc_id: String = npc_node.get_meta("npc_id")
	var r := World.interact_npc(npc_id)
	if not r.get("ok", false):
		hud.toast("Cannot talk: %s" % r.get("code", "?"))
		return
	hud.show_dialogue(npc_node.get_meta("npc_name"), World.dialogue(npc_id), _offered_mission_by(npc_id))

func _offered_mission_by(npc_id: String) -> Dictionary:
	for m in World.state.get("missions", []):
		if m.get("offering_npc_id", "") == npc_id and m.get("status", "") == "Available" and m.get("blocked_reason", "") == "":
			return m
	return {}

func try_accept_mission(mission_id: String) -> void:
	var r := World.accept_mission(mission_id)
	hud.toast("Mission accepted" if r.get("ok", false) else "Cannot accept: %s %s" % [r.get("code", ""), r.get("message", "")])

func try_enter_vehicle(vehicle_node: EFVehicle) -> void:
	var r := World.enter_vehicle(vehicle_node.vehicle_id, vehicle_node.control_seat)
	if not r.get("ok", false):
		hud.toast("Cannot board: %s — %s" % [r.get("code", ""), r.get("message", "")])
		return
	possessed = vehicle_node
	player.visible = false; player.set_physics_process(false); player.set_process_unhandled_input(false)
	vehicle_node.possess(player)
	hud.toast("Boarded %s as %s" % [vehicle_node.display_name, World.state["possession"]["role"]])

func try_exit_vehicle() -> void:
	if possessed == null:
		return
	var r := World.exit_vehicle()
	if not r.get("ok", false):
		hud.toast("Cannot exit: %s — %s" % [r.get("code", ""), r.get("message", "")])
		return
	var v: EFVehicle = possessed
	possessed = null
	v.unpossess()
	player.global_position = World.to_godot(World.state["player"]["pose"]["position"]) + Vector3(0, 1.0, 0)
	player.visible = true; player.set_physics_process(true); player.set_process_unhandled_input(true)
	player.make_current()

func cargo_action(item_index: int, deliver: bool) -> void:
	var at: Vector3 = possessed.global_position if possessed != null else player.global_position
	var poi_id := nearest_poi_id(at, 40.0)
	var node_id: String = World.economy_node_at_poi(poi_id) if poi_id != "" else ""
	if node_id == "":
		hud.toast("No facility in range")
		return
	var items: Array = World.content.get("items", [])
	if item_index >= items.size():
		return
	var item_id: String = items[item_index]["id"]
	var r: Dictionary
	if deliver:
		var carried := _carried_quantity(item_id)
		if carried <= 0:
			hud.toast("Nothing to deliver (%s)" % item_id)
			return
		r = World.deliver_cargo(node_id, item_id, carried)
	else:
		r = World.load_cargo(node_id, item_id, 1)
	hud.toast(("Delivered " if deliver else "Loaded ") + item_id if r.get("ok", false) else "%s: %s" % [r.get("code", ""), r.get("message", "")])

func item_index(item_id: String) -> int:
	var items: Array = World.content.get("items", [])
	for i in range(items.size()):
		if items[i].get("id", "") == item_id:
			return i
	return -1

func cargo_prompt() -> String:
	var parts := []
	var items: Array = World.content.get("items", [])
	for i in range(min(4, items.size())):
		parts.append("[%d] %s" % [i + 1, items[i].get("label", items[i].get("id", ""))])
	return "Load: " + " · ".join(parts) + "   [R] Deliver carried"

func _carried_quantity(item_id: String) -> int:
	if possessed != null:
		return int(World.vehicle_state(possessed.vehicle_id).get("cargo", {}).get(item_id, 0))
	return int(World.state.get("player", {}).get("inventory", {}).get(item_id, 0))

# ---------------------------------------------------------------- events & state
func _on_world_event(event_type: String, _event_id: String, payload: Dictionary) -> void:
	match event_type:
		"MissionCompleted": hud.toast("Mission complete: %s (+%d credits)" % [payload.get("title", ""), int(payload.get("credits", 0))])
		"WorldNewsPublished": hud.toast("NEWS: %s" % payload.get("headline", ""))
		"WeatherChanged": hud.toast("Weather: %s" % payload.get("new", ""))
		"InfrastructureStatusChanged": hud.toast("%s is now %s" % [payload.get("infrastructure_id", ""), payload.get("new_status", "")])

func _on_state_updated(state: Dictionary) -> void:
	for v in state.get("vehicles", []):
		if vehicles.has(v["id"]) and vehicles[v["id"]] != possessed:
			vehicles[v["id"]].sync_from_state(v)     # non-possessed vehicles follow authoritative poses

# ---------------------------------------------------------------- global input
func _unhandled_input(event: InputEvent) -> void:
	if World.backend == "":
		if event.is_action_pressed("quick_save"):
			_retry_connect()
		return
	if event.is_action_pressed("quick_save"):
		var r := World.save_game(SAVE_SLOT)
		hud.toast("Saved" if r.get("ok", false) else "Save failed: %s" % r.get("message", ""))
	elif event.is_action_pressed("quick_load"):
		var r := World.load_game(SAVE_SLOT)
		if r.get("ok", false):
			_apply_loaded_state()
			hud.toast("Loaded (tick %d)%s" % [int(r["result"].get("world_tick", 0)), "" if r["result"].get("warnings", []).is_empty() else " with warnings"])
		else:
			hud.toast("Load failed: %s" % r.get("code", ""))
	elif event.is_action_pressed("debug_overlay"):
		hud.toggle_debug()
	elif event.is_action_pressed("settings_realism"):
		realism_panel.toggle()
	elif event.is_action_pressed("pause"):
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED else Input.MOUSE_MODE_CAPTURED
	elif event.is_action_pressed("enter_exit_vehicle"):
		if possessed != null:
			try_exit_vehicle()
		else:
			var v: EFVehicle = nearest_vehicle(player.global_position, 8.0)
			if v != null:
				try_enter_vehicle(v)
	elif event.is_action_pressed("cargo_deliver"):
		for i in range(4):
			if _carried_quantity(World.content["items"][i]["id"]) > 0:
				cargo_action(i, true)
	else:
		for i in range(4):
			if event.is_action_pressed("cargo_%d" % (i + 1)):
				cargo_action(i, false)

func _apply_loaded_state() -> void:
	# Rebuild presentation from the authoritative snapshot (TDD §10: presentation actors are disposable).
	var st := World.state
	for v in st.get("vehicles", []):
		if vehicles.has(v["id"]): vehicles[v["id"]].sync_from_state(v)
	var seated: bool = st["possession"]["mode"] == "seated"
	if possessed != null and (not seated or possessed.vehicle_id != st["possession"]["vehicle_id"]):
		possessed.unpossess(); possessed = null
		player.visible = true; player.set_physics_process(true); player.set_process_unhandled_input(true)
	if seated and possessed == null and vehicles.has(st["possession"]["vehicle_id"]):
		possessed = vehicles[st["possession"]["vehicle_id"]]
		player.visible = false; player.set_physics_process(false); player.set_process_unhandled_input(false)
		possessed.possess(player)
	if possessed == null:
		player.global_position = World.to_godot(st["player"]["pose"]["position"]) + Vector3(0, 1.0, 0)
		player.make_current()
