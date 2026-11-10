# SETTINGS > GAMEPLAY > REALISM & PHYSICS (supplement §13). Pure UI over the RealismProfileManager projection.
# Global preset + per-category preset + combat challenge; custom sliders for the selected category.
# Settings are saved by the core to <saveDir>/settings/realism.json, separate from world saves (§12).
class_name EFRealismPanel
extends Control

var panel: PanelContainer
var global_opt: OptionButton
var combat_opt: OptionButton
var category_opts: Dictionary = {}     # category -> OptionButton
var slider_box: VBoxContainer
var slider_category: OptionButton
var sliders: Dictionary = {}           # parameter -> HSlider
var status: Label
var _building := false

const CATEGORY_LABELS := {
	"infantry": "Infantry (FPS/TPS)", "wheeled": "Wheeled vehicles", "tracked": "Armored / Tanks",
	"helicopter": "Helicopters", "fixed_wing": "Aircraft", "surface_vessel": "Surface ships",
	"submarine": "Submarines", "environment": "Environmental effects"
}

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT); mouse_filter = Control.MOUSE_FILTER_IGNORE; visible = false
	panel = PanelContainer.new(); panel.anchor_left = 0.5; panel.anchor_right = 0.5; panel.anchor_top = 0.5; panel.anchor_bottom = 0.5
	panel.offset_left = -420; panel.offset_right = 420; panel.offset_top = -300; panel.offset_bottom = 300; add_child(panel)
	var vb := VBoxContainer.new(); panel.add_child(vb)
	var title := Label.new(); title.text = "SETTINGS › GAMEPLAY › REALISM & PHYSICS   (F10 to close)"; title.add_theme_font_size_override("font_size", 18); vb.add_child(title)
	var note := Label.new(); note.text = "Same game, same missions, same world — only handling, assists and feedback change."; note.modulate = Color(0.8, 0.8, 0.8); vb.add_child(note)
	global_opt = _row_option(vb, "Global preset", ["ARCADE", "BALANCED", "REALISTIC"], func(i): World.realism_set_global(global_opt.get_item_text(i)))
	combat_opt = _row_option(vb, "Combat challenge (independent)", ["EASY", "NORMAL", "HARD", "CUSTOM"], func(i): World.realism_set_combat(combat_opt.get_item_text(i)))
	vb.add_child(HSeparator.new())
	var grid := GridContainer.new(); grid.columns = 2; vb.add_child(grid)
	for cat in CATEGORY_LABELS.keys():
		var l := Label.new(); l.text = CATEGORY_LABELS[cat]; grid.add_child(l)
		var o := OptionButton.new()
		for p in ["ARCADE", "BALANCED", "REALISTIC", "CUSTOM"]:
			o.add_item(p)
		o.item_selected.connect(func(i): _on_category_selected(cat, o.get_item_text(i)))
		grid.add_child(o); category_opts[cat] = o
	vb.add_child(HSeparator.new())
	var hb := HBoxContainer.new(); vb.add_child(hb)
	var sl := Label.new(); sl.text = "Custom sliders for:"; hb.add_child(sl)
	slider_category = OptionButton.new()
	for cat in CATEGORY_LABELS.keys():
		slider_category.add_item(cat)
	slider_category.item_selected.connect(func(_i): _rebuild_sliders())
	hb.add_child(slider_category)
	var scroll := ScrollContainer.new(); scroll.custom_minimum_size = Vector2(800, 200); vb.add_child(scroll)
	slider_box = VBoxContainer.new(); slider_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL; scroll.add_child(slider_box)
	var buttons := HBoxContainer.new(); vb.add_child(buttons)
	_button(buttons, "Reset category to preset", func(): World.realism_reset(slider_category.get_item_text(slider_category.selected)))
	_button(buttons, "Reset ALL to default", func(): World.realism_reset(""))
	_button(buttons, "Save profile", _save_profile)
	status = Label.new(); status.modulate = Color(0.7, 1.0, 0.7); vb.add_child(status)
	World.realism_updated.connect(_refresh)
	if not World.realism.is_empty():
		_refresh(World.realism)

func _row_option(parent: Control, text: String, items: Array, on_selected: Callable) -> OptionButton:
	var hb := HBoxContainer.new(); parent.add_child(hb)
	var l := Label.new(); l.text = text; l.custom_minimum_size.x = 260; hb.add_child(l)
	var o := OptionButton.new()
	for it in items:
		o.add_item(it)
	o.item_selected.connect(func(i):
		if not _building:
			on_selected.call(i))
	hb.add_child(o); return o

func _button(parent: Control, text: String, cb: Callable) -> void:
	var b := Button.new(); b.text = text; b.pressed.connect(cb); parent.add_child(b)

func toggle() -> void:
	visible = not visible
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if visible else Input.MOUSE_MODE_CAPTURED
	if visible:
		_rebuild_sliders()

func _on_category_selected(cat: String, preset: String) -> void:
	if _building:
		return
	if preset == "CUSTOM":
		slider_category.select(CATEGORY_LABELS.keys().find(cat))
		_rebuild_sliders()
	else:
		World.realism_set_category(cat, preset)

func _refresh(realism: Dictionary) -> void:
	_building = true
	var settings: Dictionary = realism.get("settings", {})
	_select_text(global_opt, settings.get("global_preset", "BALANCED"))
	_select_text(combat_opt, settings.get("combat_challenge", "NORMAL"))
	for cat in category_opts.keys():
		var resolved: Dictionary = realism.get("resolved", {}).get(cat, {})
		_select_text(category_opts[cat], resolved.get("preset_label", "BALANCED"))
	_building = false
	if visible:
		_rebuild_sliders()

func _select_text(o: OptionButton, text: String) -> void:
	for i in range(o.item_count):
		if o.get_item_text(i) == text:
			o.select(i)
			return

func _rebuild_sliders() -> void:
	for c in slider_box.get_children():
		c.queue_free()
	sliders.clear()
	var cat := slider_category.get_item_text(slider_category.selected)
	var resolved: Dictionary = World.realism.get("resolved", {}).get(cat, {})
	var descs: Dictionary = World.realism.get("parameter_descriptions", {})
	var values: Dictionary = resolved.get("values", {})
	var overridden: Array = resolved.get("overridden", [])
	for param in values.keys():
		var hb := HBoxContainer.new(); slider_box.add_child(hb)
		var l := Label.new(); l.text = "%s%s" % [descs.get(param, {}).get("en", param), " *" if param in overridden else ""]; l.custom_minimum_size.x = 320
		l.tooltip_text = descs.get(param, {}).get("el", ""); hb.add_child(l)
		var s := HSlider.new(); s.min_value = 0; s.max_value = 100; s.step = 1; s.value = int(values[param]); s.custom_minimum_size.x = 300
		s.drag_ended.connect(func(changed):
			if changed:
				World.realism_set_override(cat, param, int(s.value)))
		hb.add_child(s); sliders[param] = s
		var v := Label.new()
		v.text = str(int(values[param]))
		s.value_changed.connect(func(val):
			v.text = str(int(val)))
		hb.add_child(v)

func _save_profile() -> void:
	World.realism_save()
	status.text = "Profile saved (settings/realism.json)"
