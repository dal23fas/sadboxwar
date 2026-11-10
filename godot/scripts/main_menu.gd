# EFMainMenu — title screen: New Campaign / Continue / Load / Settings / Quit (master prompt §32 items 1-2, 18-19).
class_name EFMainMenu
extends Control

signal new_game
signal load_slot(slot: String)
signal open_settings
signal quit_game

var panel: PanelContainer
var status: Label
var slots_box: VBoxContainer

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	var bg := ColorRect.new(); bg.color = Color(0.06, 0.09, 0.12, 1.0); bg.set_anchors_preset(Control.PRESET_FULL_RECT); add_child(bg)
	panel = PanelContainer.new(); panel.anchor_left = 0.5; panel.anchor_right = 0.5; panel.anchor_top = 0.5; panel.anchor_bottom = 0.5
	panel.offset_left = -260; panel.offset_right = 260; panel.offset_top = -240; panel.offset_bottom = 240; add_child(panel)
	var vb := VBoxContainer.new(); vb.add_theme_constant_override("separation", 10); panel.add_child(vb)
	var title := Label.new(); title.text = "EARTH FRONTIER"; title.add_theme_font_size_override("font_size", 40); title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER; vb.add_child(title)
	var sub := Label.new(); sub.text = "DYNAMIC OPERATIONS — Asterion archipelago (prototype)"; sub.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER; sub.modulate = Color(0.75, 0.8, 0.85); vb.add_child(sub)
	vb.add_child(HSeparator.new())
	_button(vb, EFText.t("new_campaign"), func(): new_game.emit())
	var cont := _button(vb, EFText.t("continue"), func(): load_slot.emit(_latest_slot()))
	cont.disabled = _latest_slot() == ""
	var lbl := Label.new(); lbl.text = EFText.t("load_slot"); vb.add_child(lbl)
	slots_box = VBoxContainer.new(); vb.add_child(slots_box)
	_refresh_slots()
	_button(vb, EFText.t("settings"), func(): open_settings.emit())
	_button(vb, EFText.t("language") + ": " + EFText.language.to_upper(), _toggle_language)
	_button(vb, EFText.t("quit"), func(): quit_game.emit())
	status = Label.new(); status.modulate = Color(0.7, 0.75, 0.8); status.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER; vb.add_child(status)
	status.text = "Core: %s" % (World.backend if World.backend != "" else "connecting…")

func _button(parent: Control, text: String, cb: Callable) -> Button:
	var b := Button.new(); b.text = text; b.pressed.connect(cb); parent.add_child(b); return b

func set_status(text: String) -> void:
	status.text = text

func _slot_files() -> Array:
	var out := []
	for s in World.list_saves():
		out.append({"slot": str(s.get("slot", "")), "time": int(s.get("modified_unix", 0)), "tick": int(s.get("world_tick", 0))})
	out.sort_custom(func(a, b): return a["time"] > b["time"])
	return out

func _latest_slot() -> String:
	var s := _slot_files()
	return s[0]["slot"] if not s.is_empty() else ""

func _refresh_slots() -> void:
	for c in slots_box.get_children():
		c.queue_free()
	var files := _slot_files()
	if files.is_empty():
		var l := Label.new(); l.text = EFText.t("no_saves"); l.modulate = Color(0.6, 0.6, 0.6); slots_box.add_child(l)
	for f in files.slice(0, 6):
		var b := Button.new()
		b.text = "  %s — %s · world tick %d" % [f["slot"], Time.get_datetime_string_from_unix_time(int(f["time"]), true), f["tick"]]
		b.alignment = HORIZONTAL_ALIGNMENT_LEFT
		b.pressed.connect(func(): load_slot.emit(f["slot"]))
		slots_box.add_child(b)

func _toggle_language() -> void:
	EFText.set_language("el" if EFText.language == "en" else "en")
	World.realism_set_accessibility("language", EFText.language)
	# rebuild the menu with the new language
	var parent := get_parent()
	var m := EFMainMenu.new()
	m.new_game.connect(func(): new_game.emit())
	m.load_slot.connect(func(slot): load_slot.emit(slot))
	m.open_settings.connect(func(): open_settings.emit())
	m.quit_game.connect(func(): quit_game.emit())
	parent.add_child(m)
	(get_tree().current_scene as EFGame).menu = m
	queue_free()
