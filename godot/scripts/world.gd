# World — autoload bridge to the authoritative EFWorldCore (TDR-002 §1).
# The ONLY place Godot talks to the core. Two backends, same JSON API:
#   1. GDExtension class "EFWorld" (production)  -> request(json) -> json
#   2. ef_server over TCP (dev fallback)          -> JSON lines on 127.0.0.1:7777
# Game scripts never implement world rules; they submit commands and read projections.
extends Node

signal connected(backend: String)
signal connection_failed(reason: String)
signal world_event(event_type: String, event_id: String, payload: Dictionary)
signal state_updated(state: Dictionary)
signal realism_updated(realism: Dictionary)

const SERVER_HOST := "127.0.0.1"
const SERVER_PORT := 7777
const TICKS_PER_REAL_SECOND := 1.0      # 1 real second = 1 world tick = 1 game minute
const POSE_REPORT_HZ := 4.0
const STATE_REFRESH_HZ := 2.0

var backend := ""                        # "gdextension" | "tcp" | ""
var ready_world := false
var content: Dictionary = {}
var state: Dictionary = {}
var realism: Dictionary = {}

var _ext = null
var _tcp: StreamPeerTCP
var _tcp_buffer := PackedByteArray()
var _seq := 0
var _tick_accum := 0.0
var _pose_accum := 0.0
var _state_accum := 0.0
var _journal_cursor := 0
var _pose_provider: Callable = Callable()

# ---------------------------------------------------------------- lifecycle
func _ready() -> void:
	InputSetup.ensure_default_actions()
	if ClassDB.class_exists("EFWorld"):
		_ext = ClassDB.instantiate("EFWorld")
		backend = "gdextension"
	else:
		_tcp = StreamPeerTCP.new()
		var err := _tcp.connect_to_host(SERVER_HOST, SERVER_PORT)
		if err != OK:
			connection_failed.emit("connect_to_host failed: %d" % err)
			return
		var waited := 0.0
		while _tcp.get_status() == StreamPeerTCP.STATUS_CONNECTING and waited < 3.0:
			_tcp.poll()
			OS.delay_msec(20)
			waited += 0.02
		if _tcp.get_status() != StreamPeerTCP.STATUS_CONNECTED:
			connection_failed.emit("ef_server not reachable on %s:%d — start build/ef_server first" % [SERVER_HOST, SERVER_PORT])
			return
		backend = "tcp"
	var ping := call_api("ping", {})
	if ping.get("ok", false):
		connected.emit(backend)
	else:
		connection_failed.emit("ping failed: %s" % str(ping))

func _process(delta: float) -> void:
	if not ready_world:
		return
	_tick_accum += delta * TICKS_PER_REAL_SECOND
	if _tick_accum >= 1.0:
		var ticks := int(floor(_tick_accum))
		_tick_accum -= ticks
		var r := call_api("advance_ticks", {"ticks": ticks})
		if r.get("ok", false) and int(r["result"].get("new_event_count", 0)) > 0:
			_drain_events()
	_pose_accum += delta
	if _pose_accum >= 1.0 / POSE_REPORT_HZ and _pose_provider.is_valid():
		_pose_accum = 0.0
		var p = _pose_provider.call()     # expects {"entity_id": String, "position": Vector3, "yaw_deg": float} or null
		if p != null:
			report_pose(p["entity_id"], p["position"], p.get("yaw_deg", 0.0))
	_state_accum += delta
	if _state_accum >= 1.0 / STATE_REFRESH_HZ:
		_state_accum = 0.0
		refresh_state()

# ---------------------------------------------------------------- transport
func call_api(op: String, args: Dictionary) -> Dictionary:
	_seq += 1
	var req := {"id": _seq, "op": op, "args": args}
	var text := JSON.stringify(req)
	var reply_text := ""
	if backend == "gdextension":
		reply_text = _ext.request(text)
	elif backend == "tcp":
		reply_text = _tcp_roundtrip(text)
	else:
		return {"ok": false, "code": "no_backend", "message": "not connected"}
	var parsed = JSON.parse_string(reply_text)
	if typeof(parsed) != TYPE_DICTIONARY:
		return {"ok": false, "code": "bad_reply", "message": reply_text.left(200)}
	return parsed

func _tcp_roundtrip(text: String) -> String:
	_tcp.poll()
	_tcp.put_data((text + "\n").to_utf8_buffer())
	var deadline := Time.get_ticks_msec() + 5000
	while Time.get_ticks_msec() < deadline:
		_tcp.poll()
		var avail := _tcp.get_available_bytes()
		if avail > 0:
			var res := _tcp.get_data(avail)
			if res[0] == OK:
				_tcp_buffer.append_array(res[1])
		var nl := _tcp_buffer.find(10)   # '\n'
		if nl >= 0:
			var line := _tcp_buffer.slice(0, nl).get_string_from_utf8()
			_tcp_buffer = _tcp_buffer.slice(nl + 1)
			return line
		OS.delay_msec(2)
	return '{"ok":false,"code":"timeout","message":"ef_server did not answer"}'

# ---------------------------------------------------------------- game flow
func start_new_game(content_path: String, presets_path: String) -> bool:
	var pr := call_api("load_presets", {"path": presets_path})
	if pr.get("ok", false):
		realism = pr["result"]
		realism_updated.emit(realism)
	else:
		push_warning("realism presets not loaded: %s" % str(pr))
	var r := call_api("start_new_game", {"content_path": content_path})
	if not r.get("ok", false):
		push_error("start_new_game failed: %s" % str(r))
		return false
	ready_world = true
	content = call_api("content", {}).get("result", {})
	_journal_cursor = 0
	refresh_state()
	return true

func save_game(slot: String) -> Dictionary:
	return call_api("save", {"slot": slot, "build_label": "godot-phase1"})

func load_game(slot: String) -> Dictionary:
	var r := call_api("load", {"slot": slot})
	if r.get("ok", false):
		_journal_cursor = int(call_api("state", {"news_count": 0}).get("result", {}).get("journal_size", 0))
		refresh_state()
	return r

func refresh_state() -> void:
	var r := call_api("state", {"news_count": 8})
	if r.get("ok", false):
		state = r["result"]
		state_updated.emit(state)

# ---------------------------------------------------------------- commands (validated by the core)
func apply(command: Dictionary) -> Dictionary:
	var r := call_api("apply", {"command": command})
	if r.get("ok", false):
		for e in r["result"].get("events", []):
			_journal_cursor += 1
			world_event.emit(e.get("event_type", ""), e.get("event_id", ""), e.get("payload", {}))
		refresh_state()
	return r

func interact_npc(npc_id: String) -> Dictionary: return apply({"type": "InteractNpc", "npc_id": npc_id})
func accept_mission(mission_id: String) -> Dictionary: return apply({"type": "AcceptMission", "mission_id": mission_id})
func enter_vehicle(vehicle_id: String, seat_id: String) -> Dictionary: return apply({"type": "EnterVehicle", "vehicle_id": vehicle_id, "seat_id": seat_id})
func exit_vehicle() -> Dictionary: return apply({"type": "ExitVehicle"})
func switch_seat(seat_id: String) -> Dictionary: return apply({"type": "SwitchSeat", "seat_id": seat_id})
func load_cargo(node_id: String, item_id: String, qty: int) -> Dictionary: return apply({"type": "LoadCargo", "node_id": node_id, "item_id": item_id, "quantity": qty})
func deliver_cargo(node_id: String, item_id: String, qty: int) -> Dictionary: return apply({"type": "DeliverCargo", "node_id": node_id, "item_id": item_id, "quantity": qty})
func set_weather(region_id: String, condition: String) -> Dictionary: return apply({"type": "SetWeather", "region_id": region_id, "condition": condition})

func report_pose(entity_id: String, position: Vector3, yaw_deg: float) -> void:
	# Low-rate, no state refresh (the state poll handles that).
	var pose := {"position": to_core(position), "rotation": [0.0, 0.0, yaw_deg]}
	var r := call_api("apply", {"command": {"type": "ReportPose", "entity_id": entity_id, "pose": pose}})
	if r.get("ok", false):
		_journal_cursor += r["result"].get("events", []).size()

func set_pose_provider(provider: Callable) -> void:
	_pose_provider = provider

func dialogue(npc_id: String) -> String:
	return str(call_api("dialogue", {"npc_id": npc_id}).get("result", {}).get("text", ""))

func debug_summary() -> String:
	return str(call_api("debug_summary", {}).get("result", {}).get("text", ""))

# ---------------------------------------------------------------- realism settings (supplement §1-3, §13)
func realism_set_global(preset: String) -> void: _realism_call("realism_set_global", {"preset": preset})
func realism_set_category(category: String, preset: String) -> void: _realism_call("realism_set_category", {"category": category, "preset": preset})
func realism_set_override(category: String, parameter: String, value: int) -> void: _realism_call("realism_set_override", {"category": category, "parameter": parameter, "value": value})
func realism_reset(category: String = "") -> void: _realism_call("realism_reset", {"category": category})
func realism_set_combat(level: String) -> void: _realism_call("realism_set_combat", {"level": level})
func realism_save() -> void: _realism_call("realism_save", {})

func _realism_call(op: String, args: Dictionary) -> void:
	var r := call_api(op, args)
	if r.get("ok", false):
		realism = r["result"]
		realism_updated.emit(realism)
	else:
		push_warning("%s rejected: %s" % [op, str(r)])

func resolved_profile(category: String) -> Dictionary:
	return realism.get("resolved", {}).get(category, {}).get("values", {})

# ---------------------------------------------------------------- helpers
func _drain_events() -> void:
	var r := call_api("events_since", {"index": _journal_cursor})
	if r.get("ok", false):
		for e in r["result"]:
			_journal_cursor += 1
			world_event.emit(e.get("event_type", ""), e.get("event_id", ""), e.get("payload", {}))

# Core is z-up metres; Godot is y-up metres.  godot = (x, z, -y)
static func to_godot(core_pos) -> Vector3:
	if core_pos == null or core_pos.size() < 3:
		return Vector3.ZERO
	return Vector3(float(core_pos[0]), float(core_pos[2]), -float(core_pos[1]))

static func to_core(p: Vector3) -> Array:
	return [p.x, -p.z, p.y]

func vehicle_state(vehicle_id: String) -> Dictionary:
	for v in state.get("vehicles", []):
		if v.get("id", "") == vehicle_id:
			return v
	return {}

func vehicle_definition(definition_id: String) -> Dictionary:
	for d in content.get("vehicle_definitions", []):
		if d.get("definition_id", "") == definition_id:
			return d
	return {}

func poi(poi_id: String) -> Dictionary:
	for p in content.get("pois", []):
		if p.get("id", "") == poi_id:
			return p
	return {}

func economy_node_at_poi(poi_id: String) -> String:
	for n in content.get("economy_nodes", []):
		if n.get("poi_id", "") == poi_id:
			return n.get("id", "")
	return ""
