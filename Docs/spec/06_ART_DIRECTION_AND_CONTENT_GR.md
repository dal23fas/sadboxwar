# EARTH FRONTIER — VISUAL, AUDIO & CONTENT DIRECTION

## 1. Ταυτότητα

**Visual premise:** αναγνωρίσιμη σύγχρονη Γη αλλά σε **πρωτότυπο φανταστικό σκηνικό**: μεσογειακό αρχιπέλαγος, πόλεις, δάση, πεδινές διαδρομές, αεροδρόμια, λιμάνια, ορεινούς οικισμούς. Αισθητική ρεαλιστική/κινηματογραφική χωρίς αντιγραφή μοντέλων, HUD ή στολών από Operation Flashpoint, Ghost Recon, Splinter Cell, Ready or Not ή άλλους τίτλους.

Στόχος: εύκολη ανάγνωση gameplay και σταθερή απόδοση. Δεν προτιμάται το υπερβολικό bloom, το θολό TAA ή το αδιάκοπο camera shake που εμποδίζει την ορατότητα.

## 2. Environment art bible

| Biome/area | Key features | Art goals | Streaming note |
|---|---|---|---|
| Coast | βράχια, αμμουδιά, αποβάθρες | θάλασσα και διακριτή ακτογραμμή | LOD shoreline |
| Town | χαμηλά κτίρια, δρόμοι, καφέ | ζωντανός πολιτικός οικισμός | instanced props |
| Forest | πεύκα, μονοπάτια, χώμα | ατμόσφαιρα και μεταβλητή ορατότητα | foliage budget |
| Highlands | λόφοι, πέτρινα εδάφη | μεγάλες αποστάσεις ορίζοντα | terrain LOD |
| Airfield | hangar, runway, service area | καθαρή πλοήγηση παιχνιδιού | modular buildings |
| Port | docks, warehouses, boats | believable ship movement | water + prop streaming |
| Interiors | offices, terminals, service rooms | interactions/dialogues | portal/interior load |

Τα assets να είναι modular: επιφάνειες, παράθυρα, πόρτες, επιγραφές και αντικείμενα που επαναχρησιμοποιούνται χωρίς να φαίνονται αδιάκριτα επαναλαμβανόμενα.

## 3. Character art

- Πρωτότυποι χαρακτήρες με διακριτές στολές ανά **φανταστική** παράταξη, χωρίς αντιγραφή πραγματικών insignia.
- Correct silhouettes για πεζό, οδηγό, πιλότο, μηχανικό, διασώστη, ναυτικό προσωπικό.
- Modular outfits, headwear και utility props.
- Animation sets: idle, walk, jog, sprint, sit, vehicle entry, exit, interact, carry-object, emote.
- Facial animation βασικών dialogue NPC.
- Non-graphic hit/recovery states.

## 4. Ground vehicles

Αρχικά τρία μοντέλα: civil off-road truck, small service vehicle, fictional tracked armored vehicle. Στην πρώτη vertical slice μόνο ένα χρειάζεται πλήρως λειτουργικό. LOD0 λεπτομέρειες κοντά, LOD1/2 απλοποίηση και collision hull separate από visible mesh. Vehicle materials: painted metal, glass, rubber, light dirt, weathering.

## 5. Aviation

- 1 fictional utility helicopter στο πρώτο playable.
- 1 fictional small aircraft στη φάση επέκτασης.
- Cockpit first-person view, external camera, readable instruments.
- Animation for rotor/gear/doors as game visuals, όχι ακριβές manual συγκεκριμένου πραγματικού μέσου.
- LODs για remote traffic και στάσιμα aircraft props.

## 6. Naval

- 1 playable fictional patrol/utility boat για vertical slice.
- 1 modular larger ship για επέκταση.
- Bridge: interactive wheel/controls ως συμβολικά UI στοιχεία, να μην αντιγράφει πραγματικό ναυτικό εξοπλισμό.
- Sea surface with scalable waves, wakes, reflection approximations και day/night.

## 7. UI color system

- Background: dark slate (`#18232B`) για panels.
- Primary text: off-white (`#E9F0F2`).
- Navigation accent: muted cyan (`#5FB8C3`).
- Warning: amber (`#E0AB52`).
- Critical state: restrained red (`#B45E61`).
- Neutral/non-combat interactions: sage green (`#88B49A`).

Κατάσταση παρατάξεων και objectives να μη βασίζεται μόνο στο χρώμα: πρόσθεσε σχήμα/icon/label. Fonts με πλήρη ελληνικά, λατινικά, αριθμούς και σωστή εμφάνιση σε διαφορετικές αναλύσεις.

## 8. Camera language

**FPS:** σταθερός ορίζοντας και σαφές interaction reticle.  
**TPS:** ελεγχόμενη απόσταση/εμπόδια/shoulder swap όπου απαιτείται.  
**Ground vehicles:** chase, cabin, cinematic camera.  
**Helicopter/plane:** cockpit, chase, optional free look.  
**Ship:** bridge, chase, map overview.  
**Command:** strategy/map UI, όχι ανεξάρτητος κόσμος.

Όλα χρησιμοποιούν τον ίδιο clock/mission/reputation indicator.

## 9. Weather / lighting

Ένας κοινός κύκλος φωτισμού και local weather zones. Rain/fog/wind affects visuals, audio, movement feel and simplified activity rules. Night scenes must remain readable; avoid pure black. Reflections as adjustable settings. Low preset without expensive volumetrics required.

## 10. Audio and music

- Adaptive original music layers: exploration, travel, civic hub, tension, debrief.
- Road/rotor/jet/ship ambiance as original or licensed sounds.
- Positional voices, subtitles, sound mix presets.
- Environmental variation (rain, indoor reverb, sea, town square).
- Mission radio snippets are **fictional narrative content** only.
- No copyrighted music, sound effects or voices copied from reference games.

## 11. Content pipeline

**Definitions → asset references → prototypes → polish:**

1. `VehicleDefinition` with compatible mesh, HUD and camera setup.
2. `FactionDefinition` with color/icon/name/reputation tags.
3. `MissionDefinition` with UI objective text/localization keys.
4. `WorldRegionDefinition` with weather/economy/faction settings.
5. Data validation before asset import: missing paths, invalid icons, duplicate IDs, unsupported LOD.
6. Quality tiers and budgets documented per scene.

## 12. Key art / promotional concept (future)

A dramatic wide image of fictional Asterion Island at dawn: civilian harbor and small patrol ship in foreground, a rescue helicopter passing above a coastal town, vehicle on winding mountain road, distant airfield, partly cloudy sky. **No real insignia, no real conflict photos, no text, no gore.** Communicate multi-role freedom and connected environment through composition.

## 13. Content priorities

Do not spend weeks designing 30 vehicles before one is playable. Quality targets: one readable town, one functional helicopter cabin, one small boat bridge, one simple ground vehicle with correct transitions; only then expand assets. Test real gameplay over still screenshots.
