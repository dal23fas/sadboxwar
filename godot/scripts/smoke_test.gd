# Headless smoke test of the presentation layer (runs in CI: godot --headless --path godot -- --smoke).
# Drives the real Game/World/Vehicle scripts through the Phase 1-2 acceptance flow using the same
# functions the player triggers, then exits 0 on PASS / 1 on FAIL. No rendering needed.
class_name EFSmokeTest
extends Node

var game: EFGame
var failures: Array[String] = []
var steps := 0

func _init(g: EFGame) -> void:
	game = g

func check(cond: bool, what: String) -> void:
	steps += 1
	if cond:
		print("[smoke] ok   %s" % what)
	else:
		failures.append(what)
		print("[smoke] FAIL %s" % what)

func teleport_player(poi_id: String, offset := Vector3(2, 1, 2)) -> void:
	var p: Vector3 = World.to_godot(World.poi(poi_id)["pose"]["position"]) + offset
	p.y = max(game.terrain.height_at(p.x, p.z), 0.0) + offset.y
	_place(p)

func teleport_to_npc(npc_id: String) -> void:
	var n: Node3D = game.npc_markers[npc_id]
	_place(n.global_position + Vector3(1.5, 0.5, 0))

func _place(p: Vector3) -> void:
	if game.possessed != null:
		game.possessed.global_position = p
		var agl: float = p.y - game.terrain.height_at(p.x, p.z)
		World.report_pose(game.possessed.vehicle_id, p, 0.0, max(0.0, agl))
	else:
		game.player.global_position = p
		World.report_pose("player_main", p, 0.0, 0.0)
	World.refresh_state()

func mission_status(id: String) -> String:
	for m in World.state.get("missions", []):
		if m.get("mission_id", "") == id:
			return str(m.get("status", ""))
	return "?"

func stock(node_id: String, item: String) -> int:
	for n in World.state.get("economy_nodes", []):
		if n.get("id", "") == node_id:
			return int(n.get("stocks", {}).get(item, 0))
	return -1

func run() -> void:
	await game.get_tree().process_frame
	var expect := OS.get_environment("EF_EXPECT_BACKEND")
	if expect != "":
		check(World.backend == expect, "backend is %s (got %s)" % [expect, World.backend])
	check(World.ready_world, "world ready")
	check(game.player != null and game.vehicles.size() >= 6, "scene built: player + %d vehicles" % game.vehicles.size())
	check(World.state.get("time", {}).get("clock_text", "") == "Day 1 08:00", "clock starts Day 1 08:00")

	check(game.terrain != null and game.terrain.height_at(0.0, 0.0) > 1.0, "terrain built: Port Lyria pad at %.1f m" % game.terrain.height_at(0.0, 0.0))
	check(game.terrain.height_at(-1500.0, 0.0) < 0.0, "open sea below sea level")
	# --- First Contact on foot
	teleport_to_npc("npc_mayor_elara")
	var npc: Node3D = game.nearest_npc(game.player.global_position, 4.0)
	check(npc != null and npc.get_meta("npc_id") == "npc_mayor_elara", "mayor in interact range")
	game.try_interact_npc(npc)
	check(game.hud.dialogue_panel.visible, "dialogue shown")
	game.hud.accept_offered_mission()
	await game.get_tree().process_frame
	check(mission_status("mission_first_contact") == "Active", "First Contact accepted")
	teleport_to_npc("npc_harbor_master")
	game.try_interact_npc(game.nearest_npc(game.player.global_position, 4.0))
	teleport_to_npc("npc_mayor_elara")
	game.try_interact_npc(game.nearest_npc(game.player.global_position, 4.0))
	check(mission_status("mission_first_contact") == "Completed", "First Contact completed")
	check(int(World.state["player"]["credits"]) == 70, "credits 70 after first mission (got %d)" % int(World.state["player"]["credits"]))

	# --- Road Supplies with the truck
	game.try_interact_npc(game.nearest_npc(game.player.global_position, 4.0))
	game.hud.accept_offered_mission()
	await game.get_tree().process_frame
	check(mission_status("mission_road_supplies") == "Active", "Road Supplies accepted")
	teleport_player("poi_depot_south", Vector3(0, 1, 3))
	var truck: EFVehicle = game.vehicles.get("vehicle_truck_001")
	game.player.global_position = truck.global_position + Vector3(3, 1, 0)
	World.report_pose("player_main", game.player.global_position, 0.0)
	game.try_enter_vehicle(truck)
	check(game.possessed == truck, "boarded truck")
	check(World.state["possession"]["role"] == "driver", "role driver")
	# drive a few physics frames with the controller to make sure it does not crash
	Input.action_press("accelerate")
	for i in range(20):
		await game.get_tree().physics_frame
	Input.action_release("accelerate")
	check(truck.speed > 0.0, "truck accelerates under input (speed %.1f)" % truck.speed)
	teleport_player("poi_port_lyria")
	var food_idx := game.item_index("food_crate")
	for i in range(6):
		game.cargo_action(food_idx, false)
	check(int(World.vehicle_state("vehicle_truck_001").get("cargo", {}).get("food_crate", 0)) == 6, "6 food crates loaded")
	var before := stock("economy_beacon_clinic", "food_crate")
	teleport_player("poi_beacon_clinic")
	game.cargo_action(food_idx, true)
	check(stock("economy_beacon_clinic", "food_crate") == before + 6, "clinic stock +6")
	teleport_player("poi_port_lyria")
	game.try_exit_vehicle()
	check(game.possessed == null and game.player.visible, "exited truck")
	teleport_to_npc("npc_mayor_elara")
	game.try_interact_npc(game.nearest_npc(game.player.global_position, 4.0))
	check(mission_status("mission_road_supplies") == "Completed", "Road Supplies completed")
	check(int(World.state["player"]["credits"]) == 190, "credits 190 (got %d)" % int(World.state["player"]["credits"]))

	# --- helicopter, boat, submarine, plane, tracked: board, run the controller, exit
	for vid in ["vehicle_helo_001", "vehicle_boat_001", "vehicle_sub_001", "vehicle_plane_001", "vehicle_tracked_001"]:
		var v: EFVehicle = game.vehicles.get(vid)
		_place(v.global_position + Vector3(3, 1, 0))
		game.try_enter_vehicle(v)
		check(game.possessed == v, "boarded %s" % vid)
		var y0: float = v.global_position.y
		Input.action_press("accelerate")
		Input.action_press("vehicle_up")
		for i in range(120):
			await game.get_tree().physics_frame
		Input.action_release("accelerate")
		Input.action_release("vehicle_up")
		check(is_finite(v.global_position.x) and is_finite(v.global_position.y), "%s position finite" % vid)
		if v.vehicle_class == "rotorcraft":
			check(v.global_position.y > y0 + 2.0, "helicopter climbed (%.1f -> %.1f)" % [y0, v.global_position.y])
			_place(v.global_position + Vector3(0, 40, 0))   # well above the airborne-exit threshold (5 m AGL)
			game.try_exit_vehicle()
			check(game.possessed == v, "exit refused while airborne")
			_place(Vector3(v.global_position.x, game.terrain.height_at(v.global_position.x, v.global_position.z), v.global_position.z))
		game.try_exit_vehicle()
		check(game.possessed == null, "exited %s" % vid)

	# --- services: damage + repair at the depot, refuel rejected when full
	var truck2: EFVehicle = game.vehicles.get("vehicle_truck_001")
	_place(World.to_godot(World.poi("poi_depot_south")["pose"]["position"]) + Vector3(0, 4, 0))
	truck2.global_position = game.player.global_position + Vector3(4, 0, 0)
	World.report_pose("vehicle_truck_001", truck2.global_position, 0.0, 0.0)
	World.apply({"type": "SetVehicleCondition", "vehicle_id": "vehicle_truck_001", "condition_normalized": 0.6, "reason_tag": "smoke"})
	game.try_service(true)
	check(float(World.vehicle_state("vehicle_truck_001").get("condition_normalized", 0)) > 0.99, "repair at depot restored condition")
	game.try_service(false)
	check(World.state.get("player", {}).get("credits", 0) > 0, "refuel/credits sane")

	# --- panels
	game.panels.toggle_journal()
	check(game.panels.journal.visible and game.panels.journal_list.get_child_count() > 5, "journal lists missions")
	game.panels.toggle_journal()
	game.panels.toggle_map()
	await game.get_tree().process_frame
	check(game.panels.map.visible, "map opens")
	game.panels.toggle_map()
	game.panels.toggle_pause()
	check(game.get_tree().paused, "pause menu pauses")
	game.panels.toggle_pause()
	check(not game.get_tree().paused, "resume")

	# --- realism settings do not touch the world
	var snap_before: String = str(World.call_api("debug_summary", {}).get("result", {}).get("text", ""))
	World.realism_set_global("ARCADE")
	check(World.realism.get("settings", {}).get("global_preset", "") == "ARCADE", "global preset ARCADE")
	World.realism_set_category("helicopter", "REALISTIC")
	check(World.resolved_profile("helicopter").get("inertia_influence", 0) == 85, "helicopter REALISTIC resolved")
	var snap_after: String = str(World.call_api("debug_summary", {}).get("result", {}).get("text", ""))
	check(snap_before == snap_after, "world unchanged by realism settings")

	# --- save / load
	var r := World.save_game("smoke")
	check(r.get("ok", false), "save ok")
	var credits := int(World.state["player"]["credits"])
	var tick := int(World.state["time"]["tick"])
	r = World.load_game("smoke")
	check(r.get("ok", false), "load ok")
	game._apply_loaded_state()
	check(int(World.state["player"]["credits"]) == credits and int(World.state["time"]["tick"]) == tick, "state identical after load")
	check(World.state.get("news", []).size() > 0, "news feed populated")

	# --- menus / panels exist and toggle without error
	game.realism_panel.toggle()
	game.realism_panel.toggle()
	game.hud.toggle_debug()
	await game.get_tree().process_frame

	print("[smoke] %d checks, %d failures" % [steps, failures.size()])
	if failures.is_empty():
		print("SMOKE PASS")
		game.get_tree().quit(0)
	else:
		for f in failures:
			print("SMOKE FAIL: %s" % f)
		game.get_tree().quit(1)
