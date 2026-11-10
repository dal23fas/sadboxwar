# Game — main scene. Title menu → world. Builds the Asterion archipelago from the content projection
# (terrain, water, structures, ambient life, NPCs, vehicles) and runs the on-foot / in-vehicle flow.
# Presentation only: positions are reported to the core, interactions become commands, the HUD reads projections.
class_name EFGame
extends Node3D

const SAVE_SLOT := "quick"

var player: EFPlayer
var hud: EFHud
var realism_panel: EFRealismPanel
var panels: EFPanels
var menu: EFMainMenu
var terrain: EFTerrain
var sky: EFSky
var ambient: EFAmbient
var world_root: Node3D
var vehicles: Dictionary = {}        # vehicle_id -> EFVehicle
var ai_vehicles: Array = []          # presentation-only traffic on the shared controller
var npc_markers: Dictionary = {}     # npc_id -> Node3D
var poi_markers: Dictionary = {}     # poi_id -> Node3D
var possessed: EFVehicle = null
var in_world := false
var smoke := false

func _ready() -> void:
	smoke = "--smoke" in OS.get_cmdline_user_args()
	hud = EFHud.new(); add_child(hud)
	realism_panel = EFRealismPanel.new(); hud.add_child(realism_panel)
	panels = EFPanels.new(); panels.process_mode = Node.PROCESS_MODE_ALWAYS; add_child(panels)
	panels.save_requested.connect(func(slot): _save(slot))
	panels.load_requested.connect(func(slot): _load(slot))
	panels.settings_requested.connect(func(): realism_panel.toggle())
	panels.main_menu_requested.connect(_to_main_menu)
	panels.quit_requested.connect(func(): get_tree().quit())
	World.world_event.connect(_on_world_event)
	World.state_updated.connect(_on_state_updated)
	var c: Dictionary = World.connect_backend()
	if not c.get("ok", false):
		if smoke:
			print("SMOKE FAIL: core not available: %s" % c.get("reason", "?"))
			get_tree().quit(1)
			return
		hud.show_fatal("CORE NOT AVAILABLE — %s\n\nThis build needs the EFWorldCore library (GDExtension) or the dev server:\n  run build\\ef_server.exe (Windows) / ./build/ef_server, then press F5 here to retry.\nSee README.md." % c.get("reason", "?"))
		return
	World.load_presets_only()
	EFText.set_language(str(World.realism.get("settings", {}).get("accessibility", {}).get("language", "en")))
	if smoke:
		_start_new_game()
		var t := EFSmokeTest.new(self)
		add_child(t)
		t.run()
	else:
		_show_main_menu()

# ---------------------------------------------------------------- menu flow
func _show_main_menu() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	menu = EFMainMenu.new()
	hud.add_child(menu)
	menu.new_game.connect(_menu_new_game)
	menu.load_slot.connect(_menu_load_slot)
	menu.open_settings.connect(func(): realism_panel.toggle())
	menu.quit_game.connect(func(): get_tree().quit())

func _menu_new_game() -> void:
	_close_menu()
	_start_new_game()

func _menu_load_slot(slot: String) -> void:
	_close_menu()
	_start_new_game()
	_load(slot)

func _close_menu() -> void:
	if menu != null:
		menu.queue_free()
		menu = null

func _to_main_menu() -> void:
	panels.close_all()
	_teardown_world()
	_show_main_menu()

func _start_new_game() -> void:
	_teardown_world()
	if not World.start_new_game():
		hud.show_fatal("start_new_game failed (see Output)")
		return
	_build_world()
	in_world = true
	hud.show_status("")
	hud.toast("Core: %s · %s · %d missions" % [World.backend, World.content.get("content_id", "?"), World.state.get("missions", []).size()])
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	if not smoke:
		_hints()

func _hints() -> void:
	var lines := [
		"Welcome to Port Lyria. Find Mayor Elara Venn (red figure by the town hall) and press E.",
		"F accepts a mission · J opens the journal · M opens the map · compass at the top points to your objective.",
		"The Ridge 4x4 is parked next to you: stand beside it and press X. W/S/A/D to drive, X to get out.",
		"Load goods with 1-4 at a facility, R delivers · T refuels · G repairs · F5/F9 quick save/load · Esc menu.",
	]
	for i in range(lines.size()):
		get_tree().create_timer(4.0 + i * 6.0).timeout.connect(hud.toast.bind(lines[i]))

func _teardown_world() -> void:
	in_world = false
	possessed = null
	if world_root != null:
		world_root.queue_free()
		world_root = null
	vehicles.clear(); npc_markers.clear(); poi_markers.clear()
	player = null; terrain = null; sky = null; ambient = null
	World.set_pose_provider(Callable())

# ---------------------------------------------------------------- scene construction
func _build_world() -> void:
	world_root = Node3D.new(); world_root.name = "World"; add_child(world_root)
	terrain = EFTerrain.new(); world_root.add_child(terrain)
	terrain.build(World.content.get("terrain", {}), World.content.get("pois", []))
	terrain.build_dressing(world_root)
	sky = EFSky.new(); world_root.add_child(sky)
	var rng := RandomNumberGenerator.new(); rng.seed = 178401
	for p in World.content.get("pois", []):
		EFStructures.build_poi(world_root, p, terrain, rng)
		var pos: Vector3 = World.to_godot(p["pose"]["position"])
		var m := Node3D.new(); m.name = "POI_" + p["id"]; m.position = Vector3(pos.x, _ground(pos.x, pos.z, pos.y), pos.z)
		m.add_child(_label(p["name"] + "\n[" + p["kind"] + "]", 9.0))
		world_root.add_child(m); poi_markers[p["id"]] = m
	for n in World.content.get("npcs", []):
		var base: Vector3 = World.to_godot(n["pose"]["position"])
		var idx := npc_markers.size()
		var pos := base + Vector3(6 + (idx % 3) * 3, 0, 6 - (idx % 2) * 4)
		pos.y = terrain.height_at(pos.x, pos.z)
		var m := Node3D.new(); m.position = pos; m.name = "NPC_" + n["id"]
		_humanoid(m, Color(0.85, 0.35, 0.3) if n.get("faction_id", "") == "faction_commonwealth" else (Color(0.3, 0.45, 0.85) if n.get("faction_id", "") == "faction_northshore" else Color(0.9, 0.9, 0.9)))
		m.add_child(_label(n["name"], 2.4))
		m.set_meta("npc_id", n["id"]); m.set_meta("npc_name", n["name"]); m.set_meta("home", pos); m.set_meta("phase", randf() * TAU)
		world_root.add_child(m); npc_markers[n["id"]] = m
	for v in World.state.get("vehicles", []):
		var node := EFVehicle.new(); node.name = "VEH_" + v["id"]; node.terrain = terrain
		world_root.add_child(node)
		node.setup(v, World.vehicle_definition(v["definition_id"]))
		vehicles[v["id"]] = node
	ambient = EFAmbient.new(); world_root.add_child(ambient); ambient.build(World.content, terrain)
	_spawn_ai_traffic()
	player = EFPlayer.new(); player.name = "Player"; player.game = self; world_root.add_child(player)
	var sp: Vector3 = World.to_godot(World.state["player"]["pose"]["position"]) + Vector3(0, 0, 8)
	sp.y = terrain.height_at(sp.x, sp.z) + 1.0
	player.global_position = sp
	player.make_current()
	World.set_pose_provider(_pose_provider)

func _spawn_ai_traffic() -> void:
	# AI drivers use the same EFVehicle controller as the player (supplement §12).
	var routes := [
		["light_4x4_fictional", ["poi_port_lyria", "poi_beacon_clinic", "poi_ridge_mine", "poi_windfield", "poi_depot_south"], 0.7],
		["utility_truck_fictional", ["poi_depot_south", "poi_windfield", "poi_ridge_mine", "poi_beacon_clinic", "poi_port_lyria"], 0.5],
		["utility_patrol_boat_fictional", ["poi_harbor_west", "poi_pinewatch", "poi_cape_varn"], 0.8],
	]
	var n := 0
	for r in routes:
		var def: Dictionary = World.vehicle_definition(r[0])
		if def.is_empty():
			continue
		var pts: Array[Vector3] = []
		for pid in r[1]:
			var p := World.poi(pid)
			if p.is_empty():
				continue
			var gp: Vector3 = World.to_godot(p["pose"]["position"])
			if def.get("vehicle_class", "") == "naval":
				gp.y = 0.0
				gp += Vector3(-40, 0, 0)
			else:
				gp.y = terrain.height_at(gp.x, gp.z)
			pts.append(gp)
		if pts.size() < 2:
			continue
		var v := EFVehicle.new(); v.name = "AI_%d" % n; v.terrain = terrain
		world_root.add_child(v)
		v.setup({"id": "ai_%d" % n, "definition_id": r[0], "pose": {"position": World.to_core(pts[0]), "rotation": [0, 0, 0]}}, def)
		v.label.text = "traffic"
		var drv := EFAIDriver.new()
		drv.setup(pts, float(r[2]))
		v.set_ai(drv)
		ai_vehicles.append(v)
		n += 1

func _ground(x: float, z: float, fallback: float) -> float:
	return terrain.height_at(x, z) if terrain != null else fallback

func _label(text: String, height: float) -> Label3D:
	var l := Label3D.new(); l.text = text; l.position.y = height; l.font_size = 48; l.pixel_size = 0.02
	l.billboard = BaseMaterial3D.BILLBOARD_ENABLED; l.no_depth_test = true
	return l

func _humanoid(parent: Node3D, color: Color) -> void:
	var mat := StandardMaterial3D.new(); mat.albedo_color = color
	var body := MeshInstance3D.new(); var cm := CapsuleMesh.new(); cm.radius = 0.28; cm.height = 1.1; body.mesh = cm; body.position.y = 1.0; body.material_override = mat; parent.add_child(body)
	var head := MeshInstance3D.new(); var sm := SphereMesh.new(); sm.radius = 0.16; sm.height = 0.32; head.mesh = sm; head.position.y = 1.72
	var skin := StandardMaterial3D.new(); skin.albedo_color = Color(0.85, 0.7, 0.58); head.material_override = skin; parent.add_child(head)
	for x in [-0.12, 0.12]:
		var leg := MeshInstance3D.new(); var lm := CapsuleMesh.new(); lm.radius = 0.09; lm.height = 0.7; leg.mesh = lm; leg.position = Vector3(x, 0.35, 0)
		var lmat := StandardMaterial3D.new(); lmat.albedo_color = color.darkened(0.4); leg.material_override = lmat; parent.add_child(leg)

# ---------------------------------------------------------------- pose reporting (presentation -> domain, 4 Hz)
func _pose_provider():
	if possessed != null:
		var agl: float = possessed.global_position.y - terrain.height_at(possessed.global_position.x, possessed.global_position.z)
		return {"entity_id": possessed.vehicle_id, "position": possessed.global_position, "yaw_deg": possessed.rotation_degrees.y, "agl": max(0.0, agl)}
	if player != null:
		return {"entity_id": "player_main", "position": player.global_position, "yaw_deg": player.rotation_degrees.y, "agl": 0.0}
	return null

# ---------------------------------------------------------------- interaction helpers used by player/vehicle
func nearest_npc(from: Vector3, radius: float) -> Node3D:
	var best: Node3D = null; var best_d := radius
	for n in npc_markers.values():
		var d: float = from.distance_to(n.global_position)
		if d < best_d:
			best_d = d; best = n
	return best

func nearest_vehicle(from: Vector3, radius: float) -> EFVehicle:
	var best: EFVehicle = null; var best_d := radius
	for v in vehicles.values():
		var d: float = from.distance_to(v.global_position)
		if d < best_d:
			best_d = d; best = v
	return best

func nearest_poi_id(from: Vector3, radius: float) -> String:
	var best := ""; var best_d := radius
	for id in poi_markers.keys():
		var pm: Node3D = poi_markers[id]
		var d: float = Vector2(from.x, from.z).distance_to(Vector2(pm.global_position.x, pm.global_position.z))
		if d < best_d:
			best_d = d; best = id
	return best

func try_interact_npc(npc_node: Node3D) -> void:
	if npc_node == null:
		return
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
	var p: Vector3 = World.to_godot(World.state["player"]["pose"]["position"])
	p.y = max(terrain.height_at(p.x, p.z), 0.0) + 1.0
	player.global_position = p
	player.visible = true; player.set_physics_process(true); player.set_process_unhandled_input(true)
	player.make_current()

func try_service(repair: bool) -> void:
	var v: EFVehicle = possessed if possessed != null else nearest_vehicle(player.global_position, 8.0)
	if v == null:
		hud.toast("No vehicle nearby")
		return
	var r: Dictionary = World.repair_vehicle(v.vehicle_id) if repair else World.refuel_vehicle(v.vehicle_id)
	hud.toast(("Repaired " if repair else "Refuelled ") + v.display_name if r.get("ok", false) else "%s: %s" % [r.get("code", ""), r.get("message", "")])

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
	return EFText.t("load_prefix") + " · ".join(parts) + EFText.t("deliver")

func service_prompt() -> String:
	return EFText.t("service")

func cargo_action(item_index: int, deliver: bool) -> void:
	var at: Vector3 = possessed.global_position if possessed != null else player.global_position
	var poi_id := nearest_poi_id(at, 40.0)
	var node_id: String = World.economy_node_at_poi(poi_id) if poi_id != "" else ""
	if node_id == "":
		hud.toast("No facility in range")
		return
	var items: Array = World.content.get("items", [])
	if item_index < 0 or item_index >= items.size():
		return
	var item_id: String = items[item_index]["id"]
	var r: Dictionary
	if deliver:
		var carried := _carried_quantity(item_id)
		if carried <= 0:
			return
		r = World.deliver_cargo(node_id, item_id, carried)
	else:
		r = World.load_cargo(node_id, item_id, 1)
	hud.toast(("Delivered " if deliver else "Loaded ") + items[item_index].get("label", item_id) if r.get("ok", false) else "%s: %s" % [r.get("code", ""), r.get("message", "")])

func _carried_quantity(item_id: String) -> int:
	if possessed != null:
		return int(World.vehicle_state(possessed.vehicle_id).get("cargo", {}).get(item_id, 0))
	return int(World.state.get("player", {}).get("inventory", {}).get(item_id, 0))

# ---------------------------------------------------------------- events & state
func _on_world_event(event_type: String, _event_id: String, payload: Dictionary) -> void:
	match event_type:
		"MissionCompleted": hud.toast("Mission complete: %s (+%d credits)" % [payload.get("title", ""), int(payload.get("credits", 0))])
		"MissionGenerated": hud.toast("New request on the board: %s" % payload.get("template", {}).get("title", ""))
		"MissionFailed": hud.toast("Mission %s: %s" % [payload.get("mission_id", ""), payload.get("reason", "")])
		"WorldNewsPublished": hud.toast("NEWS: %s" % payload.get("headline", ""))
		"WeatherChanged": hud.toast("Weather: %s" % payload.get("new", ""))
		"InfrastructureStatusChanged": hud.toast("%s is now %s" % [payload.get("infrastructure_id", ""), payload.get("new_status", "")])
		"CampaignFlagChanged":
			if payload.get("flag", "") == "act":
				hud.toast("CAMPAIGN: %s" % payload.get("new", "").replace("_", " "))

func _on_state_updated(state: Dictionary) -> void:
	if not in_world:
		return
	for v in state.get("vehicles", []):
		if vehicles.has(v["id"]) and vehicles[v["id"]] != possessed:
			vehicles[v["id"]].sync_from_state(v)

func _process(delta: float) -> void:
	if in_world:
		_npc_schedules(delta)
	if in_world and sky != null:
		var at: Vector3 = possessed.global_position if possessed != null else player.global_position
		var yaw: float = possessed.rotation_degrees.y if possessed != null else player.rotation_degrees.y
		sky.follow(at)
		hud.update_compass(at, yaw)

func _npc_schedules(delta: float) -> void:
	# Daytime: NPCs stroll a few metres around their post; night: they stand at home. Purely presentation.
	var hour := float(World.state.get("time", {}).get("hour", 8))
	var daytime := hour >= 7.0 and hour <= 20.0
	var t := Time.get_ticks_msec() / 1000.0
	for m in npc_markers.values():
		var home: Vector3 = m.get_meta("home")
		var phase: float = m.get_meta("phase")
		var target := home
		if daytime:
			target = home + Vector3(sin(t * 0.15 + phase) * 4.0, 0, cos(t * 0.11 + phase) * 4.0)
			target.y = terrain.height_at(target.x, target.z)
		var cur: Vector3 = m.global_position
		var next := cur.lerp(target, min(1.0, delta * 0.8))
		if next.distance_to(cur) > 0.001:
			m.look_at(Vector3(next.x, cur.y, next.z), Vector3.UP)
		m.global_position = next

# ---------------------------------------------------------------- global input
func _unhandled_input(event: InputEvent) -> void:
	if World.backend == "":
		if event.is_action_pressed("quick_save"):
			var c: Dictionary = World.connect_backend()
			if c.get("ok", false):
				hud.hide_fatal()
				World.load_presets_only()
				_show_main_menu()
		return
	if not in_world:
		if event.is_action_pressed("settings_realism"):
			realism_panel.toggle()
		return
	if event.is_action_pressed("pause"):
		if realism_panel.visible:
			realism_panel.toggle()
		else:
			panels.toggle_pause()
	elif event.is_action_pressed("mission_journal"):
		panels.toggle_journal()
	elif event.is_action_pressed("world_map"):
		panels.toggle_map()
	elif event.is_action_pressed("overview"):
		panels.toggle_overview()
	elif event.is_action_pressed("profile"):
		panels.toggle_profile()
	elif event.is_action_pressed("quick_save"):
		_save(SAVE_SLOT)
	elif event.is_action_pressed("quick_load"):
		_load(SAVE_SLOT)
	elif event.is_action_pressed("debug_overlay"):
		hud.toggle_debug()
	elif event.is_action_pressed("settings_realism"):
		realism_panel.toggle()
	elif event.is_action_pressed("enter_exit_vehicle"):
		if possessed != null:
			try_exit_vehicle()
		else:
			var v: EFVehicle = nearest_vehicle(player.global_position, 8.0)
			if v != null:
				try_enter_vehicle(v)
	elif event.is_action_pressed("refuel"):
		try_service(false)
	elif event.is_action_pressed("repair"):
		try_service(true)
	elif event.is_action_pressed("cargo_deliver"):
		var items: Array = World.content.get("items", [])
		for i in range(min(4, items.size())):
			if _carried_quantity(items[i]["id"]) > 0:
				cargo_action(i, true)
	else:
		for i in range(4):
			if event.is_action_pressed("cargo_%d" % (i + 1)):
				cargo_action(i, false)

func _save(slot: String) -> void:
	var r := World.save_game(slot)
	hud.toast("Saved to %s" % slot if r.get("ok", false) else "Save failed: %s" % r.get("message", ""))

func _load(slot: String) -> void:
	var r := World.load_game(slot)
	if r.get("ok", false):
		_apply_loaded_state()
		panels.close_all()
		hud.toast("Loaded %s (tick %d)%s" % [slot, int(r["result"].get("world_tick", 0)), "" if r["result"].get("warnings", []).is_empty() else " with warnings"])
	else:
		hud.toast("Load failed: %s" % r.get("code", ""))

func _apply_loaded_state() -> void:
	# Rebuild presentation from the authoritative snapshot (presentation actors are disposable).
	var st := World.state
	for v in st.get("vehicles", []):
		if vehicles.has(v["id"]):
			vehicles[v["id"]].sync_from_state(v)
	var seated: bool = st["possession"]["mode"] == "seated"
	if possessed != null and (not seated or possessed.vehicle_id != st["possession"]["vehicle_id"]):
		possessed.unpossess(); possessed = null
		player.visible = true; player.set_physics_process(true); player.set_process_unhandled_input(true)
	if seated and possessed == null and vehicles.has(st["possession"]["vehicle_id"]):
		possessed = vehicles[st["possession"]["vehicle_id"]]
		player.visible = false; player.set_physics_process(false); player.set_process_unhandled_input(false)
		possessed.possess(player)
	if possessed == null:
		var p: Vector3 = World.to_godot(st["player"]["pose"]["position"])
		p.y = max(terrain.height_at(p.x, p.z), 0.0) + 1.0
		player.global_position = p
		player.make_current()
