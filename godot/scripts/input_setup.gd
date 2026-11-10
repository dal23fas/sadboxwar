# InputSetup — the single place where abstract actions (TDD §10) get default hardware bindings.
# Vehicle/pawn scripts only ever use action names. Rebinding UI (Phase 7) edits InputMap at runtime.
class_name InputSetup

const DEFAULTS := {
	# action            : [physical keycodes...]
	"move_forward":      [KEY_W],
	"move_back":         [KEY_S],
	"move_left":         [KEY_A],
	"move_right":        [KEY_D],
	"sprint":            [KEY_SHIFT],
	"jump":              [KEY_SPACE],
	"interact":          [KEY_E],
	"accept":            [KEY_F],
	"enter_exit_vehicle":[KEY_X],
	"switch_camera":     [KEY_C],
	"accelerate":        [KEY_W],
	"brake":             [KEY_S],
	"steer_left":        [KEY_A],
	"steer_right":       [KEY_D],
	"vehicle_up":        [KEY_SPACE],      # collective up / surface
	"vehicle_down":      [KEY_CTRL],       # collective down / dive
	"yaw_left":          [KEY_Q],
	"yaw_right":         [KEY_E],
	"cargo_panel":       [KEY_TAB],
	"mission_journal":   [KEY_J],
	"world_map":         [KEY_M],
	"settings_realism":  [KEY_F10],
	"debug_overlay":     [KEY_F3],
	"quick_save":        [KEY_F5],
	"quick_load":        [KEY_F9],
	"pause":             [KEY_ESCAPE],
	"cargo_1":           [KEY_1],
	"cargo_2":           [KEY_2],
	"cargo_3":           [KEY_3],
	"cargo_4":           [KEY_4],
	"cargo_deliver":     [KEY_R],
	"refuel":            [KEY_T],
	"overview":          [KEY_O],
	"profile":           [KEY_P],
	"repair":            [KEY_G],
}

static func ensure_default_actions() -> void:
	for action in DEFAULTS.keys():
		if not InputMap.has_action(action):
			InputMap.add_action(action)
		if InputMap.action_get_events(action).is_empty():
			for key in DEFAULTS[action]:
				var ev := InputEventKey.new()
				ev.physical_keycode = key
				InputMap.action_add_event(action, ev)
