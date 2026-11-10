# HUD — reads World.state projections only (TDD §11 command/query separation). Never a source of truth.
class_name EFHud
extends CanvasLayer

var clock_label: Label
var status_label: Label
var mission_label: Label
var news_label: Label
var prompt_label: Label
var vehicle_label: Label
var stamina_bar: ProgressBar
var toast_label: Label
var dialogue_panel: PanelContainer
var dialogue_name: Label
var dialogue_text: Label
var dialogue_hint: Label
var debug_label: Label
var fatal_label: Label
var _toasts: Array[String] = []
var _toast_timer := 0.0
var _offered_mission: Dictionary = {}

func _ready() -> void:
	var root := Control.new(); root.set_anchors_preset(Control.PRESET_FULL_RECT); root.mouse_filter = Control.MOUSE_FILTER_IGNORE; add_child(root)
	clock_label = _mk_label(root, Vector2(16, 12), 22)
	status_label = _mk_label(root, Vector2(16, 42), 16)
	mission_label = _mk_label(root, Vector2(16, 70), 16); mission_label.modulate = Color(0.95, 0.85, 0.5)
	news_label = _mk_label(root, Vector2(16, 120), 14); news_label.modulate = Color(0.8, 0.9, 1.0)
	vehicle_label = _mk_label(root, Vector2(16, 200), 16); vehicle_label.modulate = Color(0.7, 1.0, 0.8)
	prompt_label = _mk_label(root, Vector2(16, 0), 18); prompt_label.anchor_top = 1.0; prompt_label.anchor_bottom = 1.0; prompt_label.offset_top = -120; prompt_label.offset_bottom = -20
	toast_label = _mk_label(root, Vector2(0, 0), 18); toast_label.anchor_left = 0.5; toast_label.anchor_right = 0.5; toast_label.offset_left = -400; toast_label.offset_right = 400; toast_label.offset_top = 60; toast_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	stamina_bar = ProgressBar.new(); stamina_bar.min_value = 0; stamina_bar.max_value = 1; stamina_bar.value = 1; stamina_bar.show_percentage = false
	stamina_bar.position = Vector2(16, 0); stamina_bar.size = Vector2(160, 8); stamina_bar.anchor_top = 1.0; stamina_bar.anchor_bottom = 1.0; stamina_bar.offset_top = -16; stamina_bar.offset_bottom = -8; root.add_child(stamina_bar)
	dialogue_panel = PanelContainer.new(); dialogue_panel.anchor_left = 0.5; dialogue_panel.anchor_right = 0.5; dialogue_panel.anchor_top = 1.0; dialogue_panel.anchor_bottom = 1.0
	dialogue_panel.offset_left = -360; dialogue_panel.offset_right = 360; dialogue_panel.offset_top = -260; dialogue_panel.offset_bottom = -140; dialogue_panel.visible = false; root.add_child(dialogue_panel)
	var vb := VBoxContainer.new(); dialogue_panel.add_child(vb)
	dialogue_name = Label.new(); dialogue_name.add_theme_font_size_override("font_size", 18); vb.add_child(dialogue_name)
	dialogue_text = Label.new(); dialogue_text.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART; vb.add_child(dialogue_text)
	dialogue_hint = Label.new(); dialogue_hint.modulate = Color(0.9, 0.8, 0.4); vb.add_child(dialogue_hint)
	debug_label = _mk_label(root, Vector2(0, 12), 12); debug_label.anchor_left = 1.0; debug_label.anchor_right = 1.0; debug_label.offset_left = -620; debug_label.offset_right = -10; debug_label.visible = false
	fatal_label = _mk_label(root, Vector2(0, 0), 24); fatal_label.set_anchors_preset(Control.PRESET_CENTER); fatal_label.modulate = Color(1, 0.4, 0.4); fatal_label.visible = false
	World.state_updated.connect(_on_state)

func _mk_label(parent: Control, pos: Vector2, size: int) -> Label:
	var l := Label.new(); l.position = pos; l.add_theme_font_size_override("font_size", size); l.mouse_filter = Control.MOUSE_FILTER_IGNORE
	l.add_theme_color_override("font_outline_color", Color.BLACK); l.add_theme_constant_override("outline_size", 4); parent.add_child(l); return l

func _process(delta: float) -> void:
	if not _toasts.is_empty():
		_toast_timer -= delta
		if _toast_timer <= 0.0:
			_toasts.pop_front()
			_toast_timer = 3.0
	toast_label.text = "\n".join(_toasts)
	if debug_label.visible and Engine.get_process_frames() % 30 == 0:
		debug_label.text = World.debug_summary()
	if dialogue_panel.visible and Input.is_action_just_pressed("interact"):
		dialogue_panel.visible = false

func _on_state(st: Dictionary) -> void:
	var t: Dictionary = st.get("time", {})
	var region: Dictionary = st.get("regions", [{}])[0] if not st.get("regions", []).is_empty() else {}
	clock_label.text = "%s   %s   weather: %s" % [t.get("clock_text", ""), region.get("name", ""), region.get("weather", "")]
	var po: Dictionary = st.get("possession", {})
	status_label.text = "Credits %d · role %s · trust %d (%s) · stability %.2f" % [int(st.get("player", {}).get("credits", 0)), po.get("role", ""), int(region.get("local_trust", 0)), region.get("trust_band", ""), float(region.get("stability_normalized", 0))]
	var lines := []
	for m in st.get("missions", []):
		if m.get("status", "") == "Active":
			for o in m.get("objectives", []):
				if o.get("current", false):
					var desc := ""
					if o["type"] == "Interact": desc = "Talk to %s" % _npc_name(o.get("target_id", ""))
					else: desc = "Deliver %d x %s to %s (%d done)" % [int(o.get("quantity", 0)), o.get("item_id", ""), o.get("destination_node_id", ""), int(m.get("delivered_quantity", 0))]
					lines.append("▶ %s: %s" % [m.get("title", ""), desc])
	if lines.is_empty(): lines.append("No active mission — talk to an NPC")
	mission_label.text = "\n".join(lines)
	var news := []
	var arr: Array = st.get("news", [])
	for i in range(max(0, arr.size() - 4), arr.size()):
		news.append("• " + str(arr[i].get("headline", "")))
	news_label.text = "\n".join(news)

func _npc_name(npc_id: String) -> String:
	for n in World.content.get("npcs", []):
		if n.get("id", "") == npc_id:
			return n.get("name", npc_id)
	return npc_id

func set_prompt(text: String) -> void: prompt_label.text = text
func set_vehicle_readout(text: String) -> void: vehicle_label.text = text
func set_stamina(v: float) -> void: stamina_bar.value = v
func toggle_debug() -> void: debug_label.visible = not debug_label.visible
func show_fatal(text: String) -> void:
	fatal_label.visible = true
	fatal_label.text = text

func toast(text: String) -> void:
	_toasts.append(text)
	if _toasts.size() == 1:
		_toast_timer = 3.0
	if _toasts.size() > 5:
		_toasts.pop_front()
	print("[HUD] ", text)

func show_dialogue(npc_name: String, text: String, offered_mission: Dictionary) -> void:
	_offered_mission = offered_mission
	dialogue_name.text = npc_name
	dialogue_text.text = text
	dialogue_hint.text = ("[F] Accept mission: %s" % offered_mission.get("title", "")) if not offered_mission.is_empty() else "[E] Close"
	dialogue_panel.visible = true

func accept_offered_mission() -> void:
	if _offered_mission.is_empty() or not dialogue_panel.visible:
		return
	(get_tree().current_scene as EFGame).try_accept_mission(_offered_mission["mission_id"])
	_offered_mission = {}
	dialogue_panel.visible = false
