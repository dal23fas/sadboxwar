# TDR-002 — Αλλαγή engine στόχου: Godot 4

**Ημερομηνία:** 2026-11-10  
**Κατάσταση:** ACCEPTED — αντικαθιστά την §2.1 της TDR-001. Η TDR-001 παραμένει ως ιστορικό.

## 1. Απόφαση

- **Engine: Godot 4.3+ (τελευταία stable 4.x στο μηχάνημα του χρήστη).** Export: Windows/Linux/macOS **και Web (HTML5/WASM)** από τον editor.
- **Το UE5 presentation layer αφαιρέθηκε από το branch** (commit history: `bb3b30c`). Μία engine στο branch (master prompt §33).
- **Ο `EFWorldCore` παραμένει αμετάβλητος** (ίδια sources, ίδια tests). Κουμπώνει στο Godot με δύο backends που μιλούν την **ίδια JSON API** (`EFWorldCore/JsonApi.h`):
  1. **GDExtension** (`Source/EFGodot/`, C++ με godot-cpp) — production path. Δεν μεταγλωττίζεται εδώ (δεν υπάρχει godot-cpp, δίκτυο κλειστό) → UNVERIFIED, ο χρήστης το χτίζει με `scons`.
  2. **`ef_server`** (`Tools/ef_server/`, TCP JSON-lines στο localhost) — dev fallback που **δοκιμάζεται εδώ**. Το Godot μιλάει μέσω `StreamPeerTCP`. Επιτρέπει να παίξεις Phase 1 χωρίς να χτίσεις το GDExtension.
- Το GDScript game code βλέπει **ένα** autoload `World` με ίδιο interface ανεξάρτητα από backend. Καμία game logic στο GDScript δεν αντιγράφει κανόνες του core.

## 2. Αιτιολόγηση (ο χρήστης επέλεξε, εγώ συμφωνώ)

| Κριτήριο | Godot 4 | UE5 | Three.js/WebGL |
|---|---|---|---|
| Ταχύτητα επανάληψης σε αυτή τη συνεργασία (text scenes, headless CLI) | ★★★ | ★ | ★★★ |
| Έτοιμα engine systems (physics, vehicles, navmesh, audio, input, export) | ★★ | ★★★ | ☆ (όλα δικά μας) |
| Οπτική πιστότητα §33 | ★★ | ★★★ | ★ |
| Hardware απαιτήσεις dev | χαμηλές | υψηλές | χαμηλές |
| Σπίτι για τον C++ core | GDExtension | module | WASM (όχι εφικτό εδώ) |
| HTML/Web έκδοση | native export | όχι πρακτικά | native |

Για παιχνίδι που κρίνεται από τη ζωντανή simulation και όχι από το photorealism, η ταχύτητα επανάληψης βαραίνει περισσότερο.

## 3. Συμβάσεις Godot ↔ Core

- **Μονάδες:** 1 μονάδα Godot = 1 μέτρο = 1 μονάδα core.
- **Άξονες:** core είναι z-up (`position[2]` = ύψος), Godot y-up. Μετατροπή στο bridge: `godot = Vector3(core.x, core.z, -core.y)` και αντίστροφα. Μόνο το `World.gd` κάνει τη μετατροπή.
- **Ρυθμός:** `ReportPose` 4 Hz από το presentation (όχι κάθε frame). World tick: 1 πραγματικό δευτερόλεπτο = 1 tick (1 λεπτό παιχνιδιού) by default; `World.gd` συσσωρεύει delta και καλεί `advance_ticks`.
- **Input actions (TDD §10):** `move_*`, `look`, `interact`, `enter_exit_vehicle`, `switch_camera`, `accelerate`, `brake`, `steer_*`, `mission_journal`, `world_map`, `pause`, `quick_save`, `quick_load` — μόνο στο Input Map, ποτέ hardware codes σε scripts.
- **Save path:** `user://efsaves/` (Godot) ή `./Saved/` (server mode). Ίδιο αρχείο `.efsave`.

## 4. Gate για Phase 2

Ο χρήστης τρέχει το Godot project με `ef_server` (ή GDExtension) και επιβεβαιώνει το Phase 1 acceptance (Roadmap §4): start → interact NPC → camera toggle → save → load, χωρίς απώλεια player state. Στέλνει Output panel + `Saved/*.efsave`.
