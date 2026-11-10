# TDR-001 — Επιλογή Engine και στρατηγική κώδικα

**Ημερομηνία:** 2026-11-10  
**Κατάσταση:** SUPERSEDED by TDR-002 (2026-11-10): ο χρήστης επέλεξε Godot 4 πριν το gate της §5. Η ανάλυση περιβάλλοντος (§1) και η στρατηγική engine-neutral core (§2.3, §6) παραμένουν σε ισχύ.

## 1. Έλεγχος περιβάλλοντος (πραγματικά ευρήματα, όχι υποθέσεις)

Έλεγχος που εκτελέστηκε στο περιβάλλον ανάπτυξης του Claude (Linux, 2 vCPU, 7.8 GB RAM, χωρίς GPU `/dev/dri`, χωρίς δικτυακή εγκατάσταση πακέτων):

| Εργαλείο | Διαθέσιμο | Σημείωση |
|---|---|---|
| Unreal Engine 5 (UnrealEditor / UBT) | **ΟΧΙ** | Δεν υπάρχει εγκατάσταση, δεν μπορεί να κατέβει (δίκτυο κλειστό), δεν υπάρχει GPU |
| Godot 4.x | **ΟΧΙ** | Δεν υπάρχει binary, δεν μπορεί να κατέβει |
| gcc 13.3 / clang / cmake 3.28 / ctest | ΝΑΙ | Πλήρης C++17 toolchain |
| Python 3.13 + pytest, Node 22 | ΝΑΙ | Βοηθητικά εργαλεία μόνο |
| nlohmann/json, rapidjson | ΟΧΙ | Γράφτηκε ελάχιστος JSON parser/writer μέσα στο core |
| .NET / mono | ΟΧΙ | Δεν είναι εφικτό C# Godot |

**Συνέπεια:** Σε αυτό το περιβάλλον **κανένα engine project δεν μπορεί να μεταγλωττιστεί ή να εκτελεστεί**. Ό,τι απαιτεί editor/renderer παραδίδεται ως **UNVERIFIED source** με ακριβή βήματα τοπικού ελέγχου (όπως απαιτεί η §45 του master prompt).

## 2. Απόφαση

1. **Engine στόχος: Unreal Engine 5 (5.4 ή νεότερη, C++ project).** Ακολουθεί την πρώτη προτίμηση του πακέτου; καλύπτει World Partition/HLOD, Chaos Vehicles, Enhanced Input, Niagara, Water plugin — όλα απαιτούμενα από τις §05, §33, §34.
2. **Μία και μόνο engine στο branch.** Δεν δημιουργείται παράλληλο Godot project.
3. **Το authoritative world state υλοποιείται ως engine-neutral C++17 static library `EFWorldCore`** (φάκελος `Source/EFWorldCore/`), χωρίς καμία εξάρτηση από UE headers, μεταγλωττίσιμη με **`-fno-exceptions -fno-rtti`** (οι προεπιλογές του UnrealBuildTool). Ο ίδιος φάκελος χτίζεται:
   - εδώ, με CMake + CTest → **πραγματικά εκτελεσμένα tests** (αυτό το περιβάλλον),
   - μέσα στο UE project ως module `EFWorldCore` (Build.cs) → wrapped από `UEFWorldSubsystem`.
4. **Ο engine-κώδικας (Actors, Pawns, Possession, HUD) είναι λεπτό presentation layer** που υποβάλλει commands και διαβάζει read-only projections. Δεν κρατά δική του αλήθεια (§37, §4 TDD).

## 3. Γιατί όχι Godot τώρα

- Το πακέτο ορίζει Godot ως fallback **μόνο με τεκμηριωμένο περιορισμό** που εμποδίζει τον UE5 κύκλο στο **μηχάνημα του χρήστη** — όχι στο cloud περιβάλλον του Claude. Δεν υπάρχει τέτοιο στοιχείο ακόμα.
- Το Godot θα απαιτούσε GDScript ή GDExtension; κανένα από τα δύο δεν ελέγχεται εδώ περισσότερο από το UE5. Δεν προκύπτει κέρδος επαληθευσιμότητας.
- Εάν αργότερα αποδειχθεί ότι το hardware του χρήστη δεν τρέχει UE5, ο **EFWorldCore μεταφέρεται αμετάβλητος** σε GDExtension (C++). Το κόστος αλλαγής περιορίζεται στο presentation layer.

## 4. Παραδοχές (ελλείψει hardware spec)

- Ο χρήστης διαθέτει ή θα εγκαταστήσει UE 5.4+ (Windows: Visual Studio 2022 με C++ workload; Linux: clang toolchain του Epic).
- Target rig για performance budgets: **άγνωστο** — τα budgets της §40 ορίζονται μόλις δοθεί.
- Η πρώτη γεωγραφία (Asterion Island) θα χτιστεί με Landscape + placeholder meshes, όχι αγορασμένα assets.

## 5. Gate επιβεβαίωσης (υποχρεωτικό πριν το Phase 1 presentation code)

Ο χρήστης επιβεβαιώνει με ένα από τα δύο:
- **A.** `EarthFrontier.uproject` ανοίγει, το `EFWorldCore` module μεταγλωττίζεται, ο `UEFWorldSubsystem` καταγράφει `[EF] WorldCore ready` στο Output Log → συνεχίζουμε σε UE5.
- **B.** Αδυναμία UE5 (hardware/χρόνος) με συγκεκριμένο λόγο → ενεργοποιείται TDR-002 (Godot 4 GDExtension) **αντικαθιστώντας** το UE presentation layer, όχι προσθέτοντας δεύτερο.

## 6. Συνέπειες για την αρχιτεκτονική

- Core: `std::string` IDs, `Result`-based errors (όχι exceptions), `std::get_if` (όχι `std::get`), καθόλου `dynamic_cast`/`typeid`.
- File I/O του save πίσω από `ISaveStorage`; default `FileSaveStorage` (std::filesystem) για tests/CLI, UE module μπορεί να δώσει υλοποίηση με `IFileManager`.
- Μονάδες: 1 world tick = 1 λεπτό παιχνιδιού. Το engine frame **δεν** είναι το world tick (§06, §40).
