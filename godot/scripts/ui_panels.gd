# In-game panels: pause menu, mission journal, world map. All read World.state projections only.
class_name EFPanels
extends CanvasLayer

signal save_requested(slot: String)
signal load_requested(slot: String)
signal main_menu_requested
signal settings_requested
signal quit_requested

var pause: PanelContainer
var journal: PanelContainer
var journal_list: VBoxContainer
var map: PanelContainer
var map_canvas: Control
var slot_edit: LineEdit
var _map_state: Dictionary = {}

const SLOTS := ["slot1", "slot2", "slot3", "quick"]

func _ready() -> void:
	layer = 20
	# ---- pause menu
	pause = _panel(-220, 220, -230, 230)
	var vb := VBoxContainer.new(); vb.add_theme_constant_override("separation", 8); pause.add_child(vb)
	var t := Label.new(); t.text = "PAUSED"; t.add_theme_font_size_override("font_size", 26); t.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER; vb.add_child(t)
	_button(vb, "Resume", func(): toggle_pause())
	var hb := HBoxContainer.new(); vb.add_child(hb)
	var sl := Label.new(); sl.text = "Slot:"; hb.add_child(sl)
	slot_edit = LineEdit.new(); slot_edit.text = "slot1"; slot_edit.custom_minimum_size.x = 160; hb.add_child(slot_edit)
	_button(vb, "Save to slot", func(): save_requested.emit(slot_edit.text.strip_edges()))
	_button(vb, "Load slot", func(): load_requested.emit(slot_edit.text.strip_edges()))
	var quick := HBoxContainer.new(); vb.add_child(quick)
	for s in SLOTS:
		var b := Button.new()
		b.text = s
		b.pressed.connect(_pick_slot.bind(s))
		quick.add_child(b)
	_button(vb, "Settings — Realism & Physics (F10)", func(): settings_requested.emit())
	_button(vb, "Main menu", func(): main_menu_requested.emit())
	_button(vb, "Quit game", func(): quit_requested.emit())
	# ---- journal
	journal = _panel(-360, 360, -300, 300)
	var jv := VBoxContainer.new(); journal.add_child(jv)
	var jt := Label.new(); jt.text = "MISSION JOURNAL   (J to close)"; jt.add_theme_font_size_override("font_size", 20); jv.add_child(jt)
	var sc := ScrollContainer.new(); sc.custom_minimum_size = Vector2(700, 540); jv.add_child(sc)
	journal_list = VBoxContainer.new(); journal_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL; sc.add_child(journal_list)
	# ---- map
	map = _panel(-440, 440, -320, 320)
	var mv := VBoxContainer.new(); map.add_child(mv)
	var mt := Label.new(); mt.text = "WORLD MAP — Asterion archipelago   (M to close)   green=operational  red=restricted"; mv.add_child(mt)
	map_canvas = Control.new(); map_canvas.custom_minimum_size = Vector2(860, 580); map_canvas.draw.connect(_draw_map); mv.add_child(map_canvas)
	World.state_updated.connect(_on_state)

func _panel(l: float, r: float, t: float, b: float) -> PanelContainer:
	var p := PanelContainer.new()
	p.anchor_left = 0.5; p.anchor_right = 0.5; p.anchor_top = 0.5; p.anchor_bottom = 0.5
	p.offset_left = l; p.offset_right = r; p.offset_top = t; p.offset_bottom = b
	p.visible = false
	add_child(p)
	return p

func _button(parent: Control, text: String, cb: Callable) -> Button:
	var b := Button.new(); b.text = text; b.pressed.connect(cb); parent.add_child(b); return b

func _pick_slot(s: String) -> void:
	slot_edit.text = s

func any_open() -> bool:
	return pause.visible or journal.visible or map.visible

func _set_mouse() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if any_open() else Input.MOUSE_MODE_CAPTURED

func toggle_pause() -> void:
	pause.visible = not pause.visible
	journal.visible = false; map.visible = false
	get_tree().paused = pause.visible
	_set_mouse()

func close_all() -> void:
	pause.visible = false; journal.visible = false; map.visible = false
	get_tree().paused = false
	_set_mouse()

func toggle_journal() -> void:
	journal.visible = not journal.visible
	map.visible = false
	if journal.visible:
		_rebuild_journal(World.state)
	_set_mouse()

func toggle_map() -> void:
	map.visible = not map.visible
	journal.visible = false
	if map.visible:
		map_canvas.queue_redraw()
	_set_mouse()

func _on_state(st: Dictionary) -> void:
	_map_state = st
	if journal.visible:
		_rebuild_journal(st)
	if map.visible:
		map_canvas.queue_redraw()

func _accept_board_mission(mission_id: String) -> void:
	(get_tree().current_scene as EFGame).try_accept_mission(mission_id)
	_rebuild_journal(World.state)

func _npc_name(npc_id: String) -> String:
	for n in World.content.get("npcs", []):
		if n.get("id", "") == npc_id:
			return n.get("name", npc_id)
	return npc_id

func _poi_name(poi_id: String) -> String:
	var p := World.poi(poi_id)
	return p.get("name", poi_id) if not p.is_empty() else poi_id

func _rebuild_journal(st: Dictionary) -> void:
	for c in journal_list.get_children():
		c.queue_free()
	var order := {"Active": 0, "Available": 1, "Completed": 2, "Failed": 3, "Locked": 4}
	var missions: Array = st.get("missions", []).duplicate()
	missions.sort_custom(func(a, b): return order.get(a.get("status", ""), 9) < order.get(b.get("status", ""), 9))
	var act: String = st.get("campaign", {}).get("act", "")
	var head := Label.new(); head.text = "Campaign: %s" % act.replace("_", " "); head.modulate = Color(0.95, 0.85, 0.5); journal_list.add_child(head)
	for m in missions:
		var status: String = m.get("status", "")
		var title := Label.new()
		var color := Color(0.95, 0.85, 0.5) if status == "Active" else (Color(0.6, 0.9, 0.7) if status == "Completed" else (Color(0.9, 0.5, 0.5) if status == "Failed" else Color(0.85, 0.85, 0.85)))
		var blocked: String = m.get("blocked_reason", "")
		var extra := ""
		if status == "Available" and blocked != "":
			extra = "  (locked: %s)" % blocked.replace("requires_", "needs ").replace("_", " ")
		if status == "Active" and int(m.get("time_limit_ticks", 0)) > 0:
			var left := int(m.get("accepted_tick", 0)) + int(m.get("time_limit_ticks", 0)) - int(st.get("time", {}).get("tick", 0))
			extra += "  ⏱ %dh %02dm left" % [max(0, left) / 60, max(0, left) % 60]
		title.text = "[%s] %s%s%s" % [status.to_upper(), m.get("title", ""), " ★" if m.get("generated", false) else "", extra]
		title.add_theme_font_size_override("font_size", 16); title.modulate = color; journal_list.add_child(title)
		if status == "Available" and blocked == "" and str(m.get("offering_npc_id", "")) == "":
			var accept := Button.new()
			accept.text = "Accept from the board"
			accept.pressed.connect(_accept_board_mission.bind(str(m.get("mission_id", ""))))
			journal_list.add_child(accept)
		var info := Label.new()
		var classes: Array = m.get("allowed_vehicle_classes", [])
		info.text = "    issuer %s · vehicles now: %s · reward %d cr, trust %+d" % [m.get("issuer_faction_id", "").replace("faction_", ""), ", ".join(classes) if not classes.is_empty() else "none (weather?)", int(m.get("rewards", {}).get("credits", 0)), int(m.get("rewards", {}).get("local_trust_delta", 0))]
		info.modulate = Color(0.7, 0.75, 0.8); journal_list.add_child(info)
		for o in m.get("objectives", []):
			var ol := Label.new()
			var mark := "✓" if o.get("done", false) else ("▶" if o.get("current", false) else "·")
			var desc := ""
			match o.get("type", ""):
				"Interact": desc = "Talk to %s" % _npc_name(o.get("target_id", ""))
				"TransportCargo": desc = "Deliver %d × %s to %s" % [int(o.get("quantity", 0)), o.get("item_id", "").replace("_", " "), o.get("destination_node_id", "").replace("economy_", "").replace("_", " ")]
				"ReachLocation": desc = "Reach %s%s" % [_poi_name(o.get("target_id", "")), (" by %s" % o.get("required_vehicle_class", "")) if o.get("required_vehicle_class", "") != "" else ""]
			ol.text = "      %s %s" % [mark, desc]
			ol.modulate = Color(0.95, 0.95, 0.8) if o.get("current", false) else Color(0.75, 0.75, 0.75)
			journal_list.add_child(ol)

# ---------------------------------------------------------------- map (core x/y → canvas)
func _draw_map() -> void:
	var st := _map_state
	var c := map_canvas
	var W := c.size.x; var H := c.size.y
	c.draw_rect(Rect2(Vector2.ZERO, c.size), Color(0.08, 0.2, 0.32))
	var minx := -1600.0; var maxx := 3000.0; var miny := -1400.0; var maxy := 2200.0
	var sx := W / (maxx - minx); var sy := H / (maxy - miny); var s: float = min(sx, sy)
	var X := func(x: float) -> float: return (x - minx) * s + (W - (maxx - minx) * s) * 0.5
	var Y := func(y: float) -> float: return H - ((y - miny) * s + (H - (maxy - miny) * s) * 0.5)
	for isl in World.content.get("terrain", {}).get("islands", []):
		if float(isl.get("height", 0)) <= 0.0:
			continue
		var cx: float = X.call(float(isl["center"][0])); var cy: float = Y.call(float(isl["center"][1]))
		var rx: float = float(isl["radius"][0]) * s; var ry: float = float(isl["radius"][1]) * s
		var pts := PackedVector2Array()
		for i in range(40):
			var a := TAU * i / 40.0
			pts.append(Vector2(cx + cos(a) * rx * 0.92, cy + sin(a) * ry * 0.92))
		c.draw_colored_polygon(pts, Color(0.3, 0.45, 0.28))
	for road in World.content.get("terrain", {}).get("roads", []):
		var a := World.poi(road[0]); var b := World.poi(road[1])
		if a.is_empty() or b.is_empty():
			continue
		c.draw_line(Vector2(X.call(float(a["pose"]["position"][0])), Y.call(float(a["pose"]["position"][1]))), Vector2(X.call(float(b["pose"]["position"][0])), Y.call(float(b["pose"]["position"][1]))), Color(0.2, 0.2, 0.2), 2.0)
	var infra := {}
	for i in st.get("infrastructure", []):
		infra[i.get("poi_id", "")] = i.get("status", "")
	var font := ThemeDB.fallback_font
	for p in World.content.get("pois", []):
		var px: float = X.call(float(p["pose"]["position"][0])); var py: float = Y.call(float(p["pose"]["position"][1]))
		var col := Color(0.95, 0.8, 0.3)
		if infra.has(p["id"]):
			col = Color(0.4, 0.85, 0.55) if infra[p["id"]] == "operational" else Color(0.9, 0.35, 0.35)
		if p.get("kind", "") == "underwater_site":
			col = Color(0.5, 0.8, 0.95)
		c.draw_circle(Vector2(px, py), 6, col)
		c.draw_string(font, Vector2(px + 8, py + 4), p.get("name", ""), HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.95, 0.95, 0.95))
	for v in st.get("vehicles", []):
		var vx: float = X.call(float(v["pose"]["position"][0])); var vy: float = Y.call(float(v["pose"]["position"][1]))
		c.draw_rect(Rect2(vx - 4, vy - 4, 8, 8), Color(1, 1, 1))
		c.draw_string(font, Vector2(vx + 6, vy - 6), v.get("display_name", ""), HORIZONTAL_ALIGNMENT_LEFT, -1, 11, Color(0.85, 0.85, 0.85))
	var pp: Array = st.get("player", {}).get("effective_pose", {}).get("position", [0, 0, 0])
	var ppx: float = X.call(float(pp[0])); var ppy: float = Y.call(float(pp[1]))
	c.draw_circle(Vector2(ppx, ppy), 7, Color(0.3, 0.65, 1.0))
	c.draw_string(font, Vector2(ppx + 9, ppy + 4), "YOU", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.6, 0.85, 1.0))
	# active mission targets
	for m in st.get("missions", []):
		if m.get("status", "") != "Active":
			continue
		for o in m.get("objectives", []):
			if not o.get("current", false):
				continue
			var target_poi := ""
			if o.get("type", "") == "ReachLocation":
				target_poi = o.get("target_id", "")
			elif o.get("type", "") == "TransportCargo":
				for n in World.content.get("economy_nodes", []):
					if n.get("id", "") == o.get("destination_node_id", ""):
						target_poi = n.get("poi_id", "")
			elif o.get("type", "") == "Interact":
				for n in World.content.get("npcs", []):
					if n.get("id", "") == o.get("target_id", ""):
						target_poi = n.get("poi_id", "")
			var tp := World.poi(target_poi)
			if not tp.is_empty():
				var tx: float = X.call(float(tp["pose"]["position"][0])); var ty: float = Y.call(float(tp["pose"]["position"][1]))
				c.draw_arc(Vector2(tx, ty), 14, 0, TAU, 24, Color(1, 0.85, 0.3), 2.0)
	var legend := "%s · %s" % [st.get("time", {}).get("clock_text", ""), st.get("campaign", {}).get("act", "").replace("_", " ")]
	c.draw_string(font, Vector2(10, 20), legend, HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(1, 1, 1))
