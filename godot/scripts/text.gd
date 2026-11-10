# EFText — UI localization (master prompt §32: Greek/English, separated from code). Keys → {en, el}.
# Content dialogue stays in the content JSON; this table covers HUD, menus and prompts.
class_name EFText

static var language := "en"

const T := {
	"new_campaign": {"en": "New Campaign", "el": "Νέα εκστρατεία"},
	"continue": {"en": "Continue (latest save)", "el": "Συνέχεια (τελευταίο save)"},
	"load_slot": {"en": "Load slot:", "el": "Φόρτωση θέσης:"},
	"no_saves": {"en": "  (no saves yet)", "el": "  (δεν υπάρχουν saves)"},
	"settings": {"en": "Settings — Realism & Physics", "el": "Ρυθμίσεις — Ρεαλισμός & Φυσική"},
	"quit": {"en": "Quit", "el": "Έξοδος"},
	"paused": {"en": "PAUSED", "el": "ΠΑΥΣΗ"},
	"resume": {"en": "Resume", "el": "Συνέχεια"},
	"save_to_slot": {"en": "Save to slot", "el": "Αποθήκευση στη θέση"},
	"load_from_slot": {"en": "Load slot", "el": "Φόρτωση θέσης"},
	"main_menu": {"en": "Main menu", "el": "Κεντρικό μενού"},
	"quit_game": {"en": "Quit game", "el": "Έξοδος από το παιχνίδι"},
	"journal": {"en": "MISSION JOURNAL   (J to close)", "el": "ΗΜΕΡΟΛΟΓΙΟ ΑΠΟΣΤΟΛΩΝ   (J για κλείσιμο)"},
	"map": {"en": "WORLD MAP — Asterion archipelago   (M to close)   green=operational  red=restricted", "el": "ΧΑΡΤΗΣ — αρχιπέλαγος Asterion   (M για κλείσιμο)   πράσινο=λειτουργεί  κόκκινο=περιορισμένο"},
	"overview": {"en": "OPERATIONS OVERVIEW   (O to close)", "el": "ΕΠΙΧΕΙΡΗΣΙΑΚΗ ΕΙΚΟΝΑ   (O για κλείσιμο)"},
	"profile": {"en": "CHARACTER & CAREER   (P to close)", "el": "ΧΑΡΑΚΤΗΡΑΣ & ΚΑΡΙΕΡΑ   (P για κλείσιμο)"},
	"controls": {"en": "CONTROLS   (click a key to rebind, Esc cancels)", "el": "ΧΕΙΡΙΣΜΟΣ   (κλικ σε πλήκτρο για αλλαγή, Esc ακύρωση)"},
	"accept_board": {"en": "Accept from the board", "el": "Αποδοχή από τον πίνακα"},
	"no_mission": {"en": "No active mission — talk to an NPC or open the journal [J]", "el": "Καμία ενεργή αποστολή — μίλα σε NPC ή άνοιξε το ημερολόγιο [J]"},
	"talk_to": {"en": "[E] Talk to %s", "el": "[E] Μίλα με %s"},
	"board": {"en": "[X] Board %s", "el": "[X] Επιβίβαση: %s"},
	"exit_camera": {"en": "[X] Exit · [C] Camera", "el": "[X] Έξοδος · [C] Κάμερα"},
	"service": {"en": "[T] Refuel · [G] Repair", "el": "[T] Ανεφοδιασμός · [G] Επισκευή"},
	"load_prefix": {"en": "Load: ", "el": "Φόρτωση: "},
	"deliver": {"en": "   [R] Deliver carried", "el": "   [R] Παράδοση φορτίου"},
	"accept_mission": {"en": "[F] Accept mission: %s", "el": "[F] Αποδοχή αποστολής: %s"},
	"close": {"en": "[E] Close", "el": "[E] Κλείσιμο"},
	"language": {"en": "Language / Γλώσσα", "el": "Γλώσσα / Language"},
	"credits": {"en": "Credits", "el": "Μονάδες"},
	"role": {"en": "role", "el": "ρόλος"},
	"trust": {"en": "trust", "el": "εμπιστοσύνη"},
	"stability": {"en": "stability", "el": "σταθερότητα"},
	"weather": {"en": "weather", "el": "καιρός"},
}

static func t(key: String) -> String:
	var e: Dictionary = T.get(key, {})
	return str(e.get(language, e.get("en", key)))

static func set_language(lang: String) -> void:
	language = "el" if lang == "el" else "en"
