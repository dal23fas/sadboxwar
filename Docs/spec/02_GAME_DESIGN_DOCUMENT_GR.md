# EARTH FRONTIER — GAME DESIGN DOCUMENT (GDD v0.1)
## Εξειδίκευση των gameplay μηχανισμών

**Genre:** persistent-world single-player action/simulation RPG, multi-role combined-arms sandbox.  
**Setting:** εναλλακτική σύγχρονη Γη, αρχιπέλαγος ASTERION, πρωτότυπες παρατάξεις και τοποθεσίες.  
**Design restriction:** σύγκρουση και όπλα μόνο ως αφαιρετικό, μη εκπαιδευτικό videogame gameplay· χωρίς επιχειρησιακά εφαρμόσιμες τακτικές.

## 1. High concept

Ο παίκτης είναι ένας άνθρωπος μέσα σε έναν πολυεπίπεδο κόσμο και μπορεί να αναλάβει διαφορετικές ειδικότητες. Τα οχήματα δεν αντικαθιστούν τον χαρακτήρα: αποτελούν μεταβαλλόμενους χώρους δράσης ενός persistent avatar. Ο ίδιος παγκόσμιος χρόνος και οι ίδιες πολιτικές/οικονομικές εξελίξεις συνεχίζουν από το cockpit ελικοπτέρου έως τη γέφυρα πλοίου και από τον οδικό χάρτη έως το FPS/TPS.

### Τέσσερις σχεδιαστικοί πυλώνες

1. **Freedom of role:** πολλά ισότιμα επαγγέλματα και gameplay modes.
2. **Systemic consequence:** κάθε ολοκλήρωση αλλάζει δεδομένα του κόσμου.
3. **Living theater:** AI, οικονομία, συγκρούσεις και πολιτικές εξελίσσονται εκτός οθόνης.
4. **Playable continuity:** φυσική επιβίβαση/αποβίβαση, κοινά mission, inventory και save.

## 2. Core gameplay loops

**Μικρός κύκλος (2–10 λεπτά):** NPC/τερματικό → επιλογή δραστηριότητας → κίνηση/interaction → completion → feedback → world event.

**Μεσαίος κύκλος (10–45 λεπτά):** ανάληψη αποστολής → επιλογή ρόλου/μέσου → μετακίνηση → δραστηριότητα στο πεδίο → επιστροφή/παράδοση → rewards/reputation.

**Μακρύς κύκλος (πολλές συνεδρίες):** αποκτώ προσβάσιμες ειδικότητες → γνωρίζω NPC και παρατάξεις → αποφασίζω ποιους υποστηρίζω → κόσμος αλλάζει → ξεκλειδώνουν νέες αφηγηματικές δυνατότητες.

**Autonomous loop:** world ticks → economy/weather/factions → emergent needs → missions/news → player response ή αυτόματη εξέλιξη χωρίς player.

## 3. Ρόλοι: matrix για το πρώτο παιχνίδι

| Ρόλος | Βασική κάμερα | Κύριο μέσο | Τύπος παιχνιδιού | Χαρακτηριστικό σύστημα |
|---|---|---|---|---|
| Πεζός | FPS/TPS | Avatar | Εξερεύνηση/διάλογοι/σενάρια | Interaction/animation |
| Αναγνωριστής | FPS/TPS | Avatar | Παρατήρηση/story puzzles | Game-defined awareness |
| Οδηγός | Chase/cabin | 4×4/φορτηγό | Μεταφορές/εξυπηρέτηση | Vehicle/cargo |
| Χειριστής τεθωρακισμένου | Cabin/external | Fictional tracked vehicle | Vehicle gameplay | Crew/condition |
| Πιλότος ελικοπτέρου | Cockpit/chase | Helicopter | Αποστολές διάσωσης/μεταφοράς | Flight/landing |
| Πιλότος αεροπλάνου | Cockpit/chase | Fixed-wing | Αεροπορική πλοήγηση/μεταφορές | Flight/navigation |
| Καπετάνιος | Bridge/chase | Ship | Ναυσιπλοΐα/εφοδιασμός | Maritime physics |
| Συντονιστής | Overhead/map | UI + NPC | Διαχείριση/ανάθεση | Strategy abstraction |

Η έλλειψη πρόσβασης σε ρόλο να εξηγείται με φανταστικά qualification/relationship conditions. Αποφύγετε αφηγηματικά αδικαιολόγητες αλλαγές ρόλων.

## 4. Μηχανισμός possession και seat transitions

**Actors:** PlayerAvatar, VehicleActor, SeatSlot, RoleAuthorization, CameraRig, InputProfile.

**Κύκλος:** InWorld → InteractionPrompt → AuthorizationCheck → TransitionAnimation → SeatPossession → UpdatedCamera/Input → ExitRequest → SafeExitPlacement → InWorld.

**Κανόνες:**

- Αν δεν υπάρχει έγκυρο `SeatSlot`, απορρίπτεται με ορατό μήνυμα.
- Το inventory παραμένει στον PlayerProfile και δεν αντιγράφεται στο VehicleActor.
- Η αποστολή συνδέεται με το mission ID και όχι με την τρέχουσα κάμερα.
- Η κατοχή οχήματος δεν αποσυνδέει το weather/audio/news world clock.
- Μετά από save/load δεν εμφανίζεται duplicated avatar.
- Σε μη ασφαλή ή μη έγκυρη έξοδο εφαρμόζεται φιλικό game-state fallback, όχι αυθαίρετο teleport χωρίς feedback.

## 5. World design και συστηματική γεωγραφία

Αρχική playable περιοχή: **Asterion Island**, φανταστική, με κωμόπολη **Port Lyria**, λιμάνι **Harbor West**, μικρό αεροδρόμιο **Windfield**, ορεινό οικισμό **Pinewatch**, σταθμό βοήθειας **Beacon Clinic** και υπαίθριο χώρο συντήρησης **Depot South**. Όλα αποτελούν επινοημένες τοποθεσίες και κανένα δεν αντιστοιχεί σε πραγματικό στρατιωτικό στόχο.

**Region fields:** population_index, civic_stability, faction_control, support_index, weather_zone_id, economy_node_id, transportation_access, infrastructure_status, active_story_flags.

Όταν μια υποδομή αλλάζει, αλλάζει η διαθεσιμότητα δραστηριοτήτων (π.χ. κακοκαιρία → κλειστό λιμάνι → νέα αποστολή βοήθειας).

## 6. Combat and conflict design — abstract game systems

Ο στόχος είναι δραματική ένταση χωρίς ρεαλιστική εφαρμογή βίας. Fictional combat events με διακριτά game states: Ready, Engaged, Disengaged, Disabled, Recovering. Για όλα τα οπτικά εφέ προτείνεται non-graphic αισθητική. Μην αναπαράγεις αληθινή βαλλιστική/τακτικές εμπλοκής, πραγματικές ευπάθειες οχημάτων, επιχειρησιακή επίγνωση ή ακριβείς δυνατότητες οπλικών συστημάτων.

**Game parameters:** rating-based power/defense, heat/energy/ammo as abstract consumables, movement responsiveness, visual feedback, gameplay cooldowns, score, vehicle condition, assist level. Όλα data-driven και ρυθμιζόμενα μόνο για τη διασκέδαση/ισορροπία.

## 7. Stealth mechanics ως παιχνίδι

Τέσσερις ζώνες awareness: calm, curious, alerted, returning_to_normal. Το NPC μεταβαίνει με **επινοημένα meters** που αποτυπώνονται οπτικά και επιτρέπουν στον παίκτη να καταλάβει την εξέλιξη. Οι stealth αποστολές να προσφέρουν πολλαπλές μη βίαιες διαδρομές και όχι τεχνικές απόκρυψης από αληθινούς φρουρούς/συστήματα.

## 8. Aviation game feel

Δύο βασικά control presets: `ASSISTED` και `ADVANCED_GAME`. Εναλλαγή joystick/gamepad/keyboard, response curves και context-sensitive HUD. Ο στόχος είναι το βάρος/ρυθμός απόκρισης/κατάσταση πτήσης να διαφέρει μεταξύ ελικοπτέρου και fixed-wing. `FlightAssistComponent` ρυθμίζει τις βοήθειες χωρίς να αντικαθιστά το μοντέλο φυσικής. Η αρχική έκδοση δεν χρειάζεται ακριβή avionics υπαρκτών στρατιωτικών μέσων.

## 9. Naval game feel

Η θάλασσα προσφέρει λειτουργικά κύματα, αδράνεια πλοίων, μεταφορά φορτίου, πρόσδεση ως game interaction, bridge camera, πλήρωμα, κατάσταση συντήρησης και αποστολές διάσωσης/μεταφοράς. Προοδευτικά μεγαλύτερα σκάφη με διαφορετικές θέσεις και interiors. Αποφύγετε advanced simulation πραγματικών ναυτικών όπλων.

## 10. Population / settlements

NPC profiles: ID, display name, occupation, home region, basic schedule, work state, dialogue tags, trust score, current simplified task. Το πλήθος των simulated NPC δεν είναι ίσο με τα rendered characters. Για background: aggregated population actors με μικρούς αριθμητικούς δείκτες· κοντά στον παίκτη: representative NPC με φυσική και animations.

## 11. Factions / politics

Κάθε παράταξη έχει `relationship_matrix`, `resource_pool`, `civic_support`, `stability`, `region_claims`, `active_agenda` και `mission_templates`. Ενέργειες από campaign, world events και player μεταβάλλουν τους δείκτες. Οι επιλογές να επιτρέπουν διαλλακτικά, ουδέτερα και ανθρωπιστικά αποτελέσματα. Το conflict loop δεν πρέπει να είναι ο μοναδικός τρόπος προόδου.

## 12. Economy / progression

Credits/mission rewards για fiction economy. Προϊόντα πολιτικού χαρακτήρα όπως νερό, τρόφιμα, καύσιμα, ανταλλακτικά, υγειονομικό υλικό. Ο κάθε κόμβος έχει stock, capacity, inflow, outflow, shortage pressure. Mission reward: currency, reputation, qualification progress, narrative access, persistent community impact. Balance safeguards: caps, transaction logging, no duplicate completion, input validation.

## 13. Mission design conventions

Κάθε mission template έχει objective nodes `Interact`, `ReachZone`, `TalkToNPC`, `TransportCargo`, `Explore`, `Return`, `WaitForEvent`, `BranchChoice`. Combat-style encounters υπάρχουν ως high-level game events, χωρίς real-world operational instructions. Κάθε node έχει tracked conditions, location references, UI hints, retry/fail policy, and state updates. Ο generator επιλέγει μόνο συμβατά tasks σε διαθέσιμα οχήματα/regions.

## 14. Campaign characters και branching

Αρχικοί χαρακτήρες: Mayor Elara Venn (κοινότητα), Captain Mira Solis (λιμάνι), Pilot Tomas Vale (εναέρια μεταφορά), Engineer Ivo Renn (υποδομές), Mediator Nera Kade (ουδέτερη οργάνωση). Κάθε ένας έχει `relationship_state`, `dialogue_conditions`, `mission_chain`, `story_flags`. Η ιστορία προσαρμόζεται στην πραγματική οικονομική και πολιτική κατάσταση του κόσμου.

## 15. World-news και storytelling

Τα news items παράγονται από events και **πάντα** μπορούν να αναχθούν σε `source_event_id`. Ο τίτλος/περίληψη γράφονται από localization templates. Δεν ισχυρίζεται το news system αληθινά περιστατικά. NPC barks μπορούν να αναφέρονται σε πρόσφατα events της περιοχής.

## 16. Difficulty / accessibility

Separately adjustable: movement assist, flight assist, driving assist, combat challenge (abstract), UI guidance, stealth awareness readability, subtitles, text size, colorblind-friendly faction indicators, toggle/hold input modes, motion blur reduction, camera shake scale. Δύο presets `STORY` και `SIMULATION_FEEL`, χωρίς να δημιουργούνται δύο διαφορετικά world-state μοντέλα.

## 17. Content constraints / ethics / copyrights

- Πρωτότυπες φανταστικές παρατάξεις, πρόσωπα και χάρτες.
- Καμία άμεση αναπαραγωγή assets, UI, μουσικής ή κώδικα των τίτλων αναφοράς.
- Όχι πραγματικές βάσεις, ευαίσθητες τοποθεσίες, ενεργές συγκρούσεις ως objectives.
- Όχι γραφικές σκηνές βίας, θύματα με ρεαλιστικές λεπτομέρειες ή εργαλεία εκμάθησης χρήσης όπλων.
- Η ζωή πολιτών, η βοήθεια και οι συνέπειες είναι σημαντικά gameplay συστήματα.

## 18. MVP Definition of Done

Ο παίκτης μπορεί σε μία συνεδρία να: ξεκινήσει ως πεζός → δεχτεί αποστολή → χρησιμοποιήσει οδικό όχημα → επιβιβαστεί σε ελικόπτερο ή πλοίο → ολοκληρώσει αποστολή μεταφοράς → ενημερώσει το τοπικό stock → κερδίσει φήμη και credits → διαβάσει world news → αποθηκεύσει → φορτώσει και να δει τον ίδιο μεταβλημένο κόσμο.
