# MASTER DEVELOPMENT PROMPT ΓΙΑ CLAUDE AI
## PROJECT: EARTH FRONTIER — DYNAMIC OPERATIONS
### Ενιαίο 3D παιχνίδι σύγχρονης Γης: στρατιώτης, πιλότος, χειριστής τεθωρακισμένου, καπετάνιος, διασώστης, διοικητής

> **ΤΡΟΠΟΣ ΧΡΗΣΗΣ:** Δώσε ολόκληρο αυτό το αρχείο στο Claude ως κύρια προδιαγραφή. Τα υπόλοιπα αρχεία του πακέτου αποτελούν συμπληρωματικές, δεσμευτικές προδιαγραφές. Η λέξη «υλοποίησε» σημαίνει παραγωγή πραγματικών, ενσωματώσιμων αρχείων και δοκιμών, όχι περιγραφή ή ψευδοκώδικα. Μην ισχυριστείς ότι δοκιμάστηκε κάτι χωρίς εκτέλεση.

---

## 01. ΡΟΛΟΣ ΚΑΙ ΑΠΟΣΤΟΛΗ ΤΟΥ AI

Ενέργησε ως συντονισμένη ομάδα έμπειρων δημιουργών videogames: Game Director, Lead Game Designer, Technical Director, Engine Programmer, AI Engineer, Simulation Engineer, UI/UX Lead, Narrative Designer, Technical Artist, Sound Designer, Performance Engineer, QA Lead και Build/Release Engineer.

Θέλω να δημιουργήσουμε ένα **μεγάλο single-player, 3D, persistent-world, combined-arms action/simulation game**, που διαδραματίζεται στη **Γη**, με πρωτότυπες, φανταστικές χώρες και οργανώσεις. Ο παίκτης δεν είναι κλειδωμένος σε έναν ήρωα/ένα όχημα/ένα είδος gameplay. Μπορεί να υπηρετεί ως στρατιώτης, ειδικός επιχειρήσεων, πιλότος ελικοπτέρου ή αεροπλάνου, χειριστής τεθωρακισμένου, κυβερνήτης πλοίου, μέλος πληρώματος, διασώστης, χειριστής μεταφορών ή διοικητής μιας μεγαλύτερης δύναμης.

Πηγές **έμπνευσης για την αίσθηση**, όχι για αντιγραφή προστατευμένου περιεχομένου: *Operation Flashpoint* (ανοιχτός στρατιωτικός κόσμος), *Ghost Recon* (ομαδικό παιχνίδι), *Splinter Cell* (ατμόσφαιρα stealth), *Ready or Not* (ένταση και αλληλεπιδράσεις σε εσωτερικούς χώρους), παιχνίδια ναυτικών επιχειρήσεων όπως *Warship* και προσομοιωτές τεθωρακισμένων όπως *Tank Platoon*. Θέλω **δική μας ταυτότητα**, πρωτότυπους χαρακτήρες, υποδομές, μοντέλα, στολές, interface, ηχητικό υλικό και σενάριο.

**Απολύτως κεντρική απαίτηση:** πρόκειται για **ΕΝΑ παιχνίδι** με **ΕΝΑ world state**. Ο χρόνος, ο καιρός, οι αποστολές, η οικονομία, οι σχέσεις παρατάξεων, η κατάσταση εγκαταστάσεων, οι NPC και οι κατοχές περιοχών δεν επανεκκινούν όταν ο παίκτης αλλάζει ρόλο ή όχημα.

Η μάχη παραμένει **φανταστικό gameplay**: δεν ζητώ εκπαιδευτικό εργαλείο πραγματικών στρατιωτικών τακτικών, οδηγίες πραγματικής χρήσης όπλων, ακριβή επιχειρησιακά πρωτόκολλα ή αντιγραφή αληθινών στρατιωτικών εγκαταστάσεων. Εστιάζουμε σε καθαρά συστήματα παιχνιδιού, AI, εμβύθιση, διακλαδισμένο σενάριο και ανθρώπινες συνέπειες.

## 02. ΠΡΟΤΕΡΑΙΟΤΗΤΕΣ ΤΟΥ DESIGN

1. **Ενιαίος, ζωντανός κόσμος**: οι παρατάξεις δρουν και όταν ο παίκτης απουσιάζει.
2. **Πολλαπλοί ρόλοι**: infantry, aviation, armored, naval, support και command.
3. **Συνοχή**: το ίδιο συμβάν εμφανίζεται σωστά στον χάρτη, στην οικονομία, στις αποστολές και στους NPC.
4. **Πραγματική δυνατότητα χειρισμού**: cockpit, γέφυρα πλοίου, οδηγός οχήματος, FPS/TPS και στρατηγικός χάρτης.
5. **Πολιτικές και ανθρωπιστικές συνέπειες**: παρουσία αμάχων, αποκατάσταση και επιπτώσεις στην κοινωνία του κόσμου.
6. **Ρεαλιστική αισθητική χωρίς μη αναγκαία πολυπλοκότητα**: πρώτα playable, μετά υψηλή πιστότητα.
7. **Modular, data-driven, testable υλοποίηση**.
8. **Καμία ψεύτικη παράδοση**: τα builds, tests, αρχεία και τα screenshots είναι πραγματικά ή αναφέρονται ως μη επαληθευμένα.

## 03. ΤΑΥΤΟΤΗΤΑ ΚΟΣΜΟΥ — ΓΗ, ΑΛΛΑ ΠΡΩΤΟΤΥΠΗ

Πλαίσιο: μια **εναλλακτική σύγχρονη Γη** με ρεαλιστικό κλίμα, θάλασσα, ορεινή γεωγραφία, αγροτικές περιοχές, μητροπολιτικά κέντρα, λιμάνια και αεροδρόμια, αλλά **φανταστικά κράτη και γεωπολιτικές εντάσεις**. Δεν χρησιμοποιούμε πραγματικούς χάρτες ευαίσθητων στόχων, τρέχοντες πολέμους ή πραγματικές στρατιωτικές θέσεις ως gameplay targets.

Πρώτη περιοχή: το φανταστικό παράκτιο αρχιπέλαγος **ASTERION**, τμήμα της εναλλακτικής Γης, με νησιά, βουνά, λιμάνι, κωμόπολη, αεροδιάδρομο, οδικό δίκτυο και παράκτιες εγκαταστάσεις. Ο κόσμος αργότερα επεκτείνεται σε πολλές ηπειρωτικές περιοχές, αλλά η αρχική εκδοχή χρειάζεται μόνο ένα **πλούσιο, συνεχές playable map**.

Η γεωγραφία να είναι σχεδιασμένη ώστε να υποστηρίζει φυσικά στεριά, αέρα και θάλασσα. Το περιβάλλον να έχει λειτουργικές διαδρομές μετακίνησης, σημεία εργασίας, καταφύγια, πολιτικούς οικισμούς, περιοχές εκπαίδευσης NPC και αντικείμενα αλληλεπίδρασης.

## 04. PLAYABLE ΡΟΛΟΙ ΚΑΙ ΕΛΕΥΘΕΡΙΑ ΚΑΡΙΕΡΑΣ

Ρόλοι με κοινή πρόοδο και πρόσβαση ανάλογα με την εμπειρία, το πλαίσιο αποστολής και τις σχέσεις με την παράταξη:

- **Infantry Operator:** κίνηση σε FPS/TPS, διάλογοι, αποστολές και αλληλεπιδράσεις.
- **Recon / Stealth Specialist:** παρατήρηση, χαρτογράφηση, συλλογή στοιχείων, μη βίαιες επιλογές, detection mechanics.
- **Squad Member / Leader:** συμμετοχή σε ομάδα NPC, απλές εντολές συνεργασίας.
- **Helicopter Pilot:** μεταφορά, διάσωση, περιπολία, αεροπορικές αποστολές gameplay.
- **Fixed-wing Pilot:** αεροπλάνα, πτήση, αποστολές αναγνώρισης και σενάρια αερομαχιών.
- **Armored Vehicle Crew:** οδήγηση και διαχείριση τεθωρακισμένων/αρμάτων μέσα σε abstract παιχνίδι οχημάτων.
- **Naval Captain:** πλοήγηση, διοίκηση πληρώματος, αποστολές θαλάσσιας παρουσίας, συνοδείας και διάσωσης.
- **Boat / Coast Patrol Pilot:** χειρισμός μικρότερων πλοιαρίων και σύνδεση με παράκτιες αποστολές.
- **Logistics / Engineering Specialist:** μεταφορές, επισκευές, ανεφοδιασμός και αποκατάσταση.
- **Rescue / Medical Support:** εκκένωση και ανθρωπιστικές αποστολές με game-safe παρουσίαση.
- **Operations Commander:** εποπτικός χάρτης, πόροι, διαθεσιμότητα μονάδων και μακρο-αποφάσεις.

Δεν αποκτά ο παίκτης μαγικές ικανότητες. Ένα abstract **Qualification & Certification System** ξεκλειδώνει εξοπλισμό/μέσα μέσα στο fiction του παιχνιδιού, χωρίς να περιλαμβάνει πραγματική στρατιωτική εκπαίδευση.

## 05. ΧΑΡΤΗΣ ΚΑΙ ΔΟΜΗ ΤΟΥ OPEN WORLD

Υποστήριξε:

- Επίγεια έκταση με βουνά, δάση, αγροτικές περιοχές και οικισμούς.
- Δρόμους, γέφυρες, τούνελ, λιμάνια, αποβάθρες, αεροδιαδρόμους.
- Θαλάσσιες εκτάσεις, ακτές, νησιά και δυναμικό καιρό.
- Εσωτερικούς χώρους επιλεγμένων κτιρίων με interactive doors, διαλόγους, terminals.
- Μετακινούμενους NPC, πολιτική κυκλοφορία, εμπορικές δραστηριότητες.
- Regions / sectors με ιδιότητες κατοχής, σταθερότητας, υποδομών και ασφάλειας.
- Διαφορετικές καταστάσεις λειτουργίας των οικισμών ανάλογα με τα γεγονότα.

Χρησιμοποίησε **world partition/streaming**, LOD και background simulation. Απαγορεύεται η σπατάλη πόρων σε off-screen full-resolution NPC και physics.

## 06. ΖΩΝΤΑΝΟΣ ΚΟΣΜΟΣ ΧΩΡΙΣ ΤΟΝ ΠΑΙΚΤΗ

Αυτό είναι το βασικό USP. Το παιχνίδι πρέπει να έχει **Dynamic Earth State Simulation** με clock/tick ανεξάρτητο από το framerate.

Ο κόσμος παρακολουθεί:

- Περιφερειακό έλεγχο και ασφάλεια.
- Φήμη και διπλωματικές σχέσεις.
- Οικονομική δραστηριότητα και διαθεσιμότητα πόρων.
- Λειτουργική κατάσταση λιμανιών, αεροδρομίων και μεταφορών.
- Διαθεσιμότητα αποστολών και δυνάμεων.
- Πολιτικό πληθυσμό, ανάγκες και μετακινήσεις σε αφαιρετικό επίπεδο.
- Καιρικές συνθήκες, ώρες της ημέρας, ορατότητα παιχνιδιού.
- Εξέλιξη narrative arcs και γεγονότων.

Κανένας NPC σε μακρινή περιοχή δεν χρειάζεται να περπατά με πλήρες animation. Μακριά από τον παίκτη εφαρμόζονται **statistical simulation**, aggregated events και deterministic seeds. Κοντά του χρησιμοποιούνται πραγματικά playable entities.

## 07. ΕΝΙΑΙΑ ΤΑΥΤΟΤΗΤΑ ΠΑΙΚΤΗ ΚΑΙ ΑΛΛΑΓΗ ΡΟΛΩΝ

Το player account/avatar έχει ενιαίο ID, inventory, qualification flags, reputation, career history και campaign flags. Τα οχήματα και οι ρόλοι είναι **possessable entities** με κοινό interface, όχι ξεχωριστές εφαρμογές.

Πρέπει να υποστηρίζεται:

1. Περπάτημα προς όχημα/σκάφος.
2. Interaction με διαθέσιμη θέση/ρόλο.
3. Επιβίβαση με camera transition.
4. Χειρισμός του νέου entity.
5. Διατήρηση αποστολής και world-state.
6. Αποβίβαση στη σωστή θέση και συνθήκες.
7. Επιστροφή σε FPS/TPS χωρίς reload του συνολικού κόσμου.

Προαιρετικές εναλλαγές θέσεων στο ίδιο όχημα (pilot/crew/captain) πρέπει να ελέγχονται από seat rules και να διατηρούν τη φυσική/κατάσταση του entity.

## 08. CHARACTER MOVEMENT ΚΑΙ FPS/TPS

Υλοποίησε ένα συνεκτικό character controller:

- Walk / run / sprint με κόπωση ως ήπιο gameplay resource.
- Jump, crouch, prone εάν συμφωνεί με την αισθητική.
- Interactions με NPC, πόρτες, οχήματα, terminals, αντικείμενα.
- First-person και third-person κάμερα με άμεση εναλλαγή.
- Contextual animations και animation blending.
- Equipment slots, inventory, personal protection ως αφαιρετικά game stats.
- Κίνηση σε ανώμαλο έδαφος, διαφορετικές επιφάνειες και εσωτερικούς χώρους.
- Accessibility options και customizable controls.

Η συμπεριφορά του χαρακτήρα πρέπει να λειτουργεί σε single authoritative player-state, χωρίς «διπλά» inventory συστήματα ανά mode.

## 09. STEALTH / RECON GAMEPLAY (ΑΦΑΙΡΕΤΙΚΟ ΚΑΙ ΑΣΦΑΛΕΣ)

Εμπειρία έντασης, παρατήρησης και προσεκτικής μετακίνησης, χωρίς να μετατρέπεται σε πραγματικό εγχειρίδιο αποφυγής εντοπισμού.

- Game-defined visibility/noise meters.
- Awareness levels: unaware, curious, alert, searching, disengaged.
- Friendly/non-hostile dialogue alternatives.
- Object interactions με game-specific κανόνες.
- Information gathering και environmental storytelling.
- Game-only detection logic με telegraphed cues για δίκαιη εμπειρία.
- Optional non-combat completion routes και mission scoring.

Μην εισαγάγεις πραγματικούς κανόνες διάρρηξης, παράκαμψης φύλαξης, χακαρίσματος ή τακτικής διείσδυσης. Τα stealth puzzles πρέπει να είναι επινοημένα, ισορροπημένα και αμιγώς παιχνιδικά.

## 10. EQUIPMENT ΚΑΙ COMBAT GAMEPLAY

Θέλω διαφορετικά gameplay archetypes εξοπλισμού και απολύτως φανταστική εξισορρόπηση (recoil feel, rate/energy/ammo abstraction, animation states, game-defined range bands), χωρίς πραγματικές τεχνικές οδηγίες χρήσης όπλων ή ακριβή πραγματικά βαλλιστικά μοντέλα.

- Primary/secondary fictional game equipment categories.
- Non-lethal/interaction tools όπου ταιριάζουν στη σχεδίαση.
- Protective equipment και role-specific utility items.
- Cosmetic personalization.
- Weapon/equipment progression με trade-offs.
- Clear friend/neutral/hostile identification του game state.
- Non-graphic hit feedback, scoring, downed/recovery states.
- Difficulty presets και δυνατότητες απενεργοποίησης έντονων οπτικών στοιχείων.

Η μάχη είναι **φανταστικός μηχανισμός ψυχαγωγίας**. Να μη δημιουργούνται ακριβείς προσομοιώσεις πραγματικών όπλων, ρεαλιστικά στοιχεία διατρητικότητας/αποτελεσματικότητας ή επιχειρησιακές οδηγίες για πραγματικές συγκρούσεις.

## 11. GROUND VEHICLES — ΑΠΟ JEEP ΕΩΣ ARMORED

Κατηγορίες:

- Civilian car / utility truck.
- 4x4 / off-road transport.
- Ambulance / rescue vehicle.
- Logistics truck.
- Light armored transport.
- Tracked armored vehicle / fictional main battle tank.
- Amphibious vehicle.
- Engineering/support vehicle.

Κάθε τύπος να έχει: μάζα παιχνιδιού, steering response, acceleration, braking, terrain handling, seat slots, passenger capacity, cargo capacity, condition, fuel/resource abstraction, upgrade slots.

**Vehicle gameplay:** είσοδος/έξοδος, εναλλαγή κάμερας, dashboard, crew interactions, abstract damage model, repair loop, mission integration. Τα άρματα πρέπει να νιώθουν βαριά και ξεχωριστά από πολιτικά οχήματα, χωρίς να αναπαράγουν πραγματικές διαδικασίες χειρισμού ή στοχοποίησης.

## 12. HELICOPTERS

Πολλαπλές playable κατηγορίες:

- Light helicopter.
- Utility / transport helicopter.
- Search-and-rescue helicopter.
- Heavy-lift helicopter.
- Fictional armed support helicopter.

Χειρισμός: collective/throttle-style game inputs, yaw/pitch/roll ως videogame controls, assisted/advanced mode, εξωτερική και cockpit κάμερα, landing/boarding, μεταφορά NPC/cargo, αλληλεπίδραση με καιρό, εμφανή flight-state indications.

Ζητώ **αίσθηση πτήσης** με σαφή feedback, υποστήριξη HOTAS/gamepad/keyboard, αλλά όχι πραγματικό εκπαιδευτικό flight manual ή χειρισμούς συγκεκριμένου υφιστάμενου στρατιωτικού ελικοπτέρου.

## 13. AIRPLANES / JETS / AIR OPERATIONS

Κατηγορίες:

- Light prop / training-style aircraft (fictional).
- Transport aircraft.
- Patrol aircraft.
- Fictional multirole jet.
- Fictional carrier-capable jet, εφόσον το υποστηρίξει ο κόσμος.

Μηχανισμοί: runway interactions ως game states, assisted flight, cinematic high-speed travel, atmospheric weather, navigation waypoints, cockpit gauges, AI traffic, scripted/campaign και systemic missions, safe takeoff/landing feedback, hangar/maintenance UI.

Δύο επίπεδα μοντέλου: **Accessible** και **Advanced Gameplay**, με κοινή φυσική βάση και παραμετρικά assists. Δεν απαιτώ πιστή αναπαραγωγή πραγματικών πολεμικών συστημάτων ή τεχνικών αποστολής.

## 14. NAVAL SHIPS ΚΑΙ ΚΑΠΕΤΑΝΙΟΣ

Να μπορώ να επιβιβαστώ σε πλοίο, να βρίσκομαι στη γέφυρα, να χειρίζομαι το σκάφος και να διαχειρίζομαι αποστολές.

Κατηγορίες:

- Patrol boat.
- Rescue vessel.
- Supply ship.
- Ferry / cargo vessel.
- Fictional corvette.
- Fictional frigate.
- Fictional larger command ship.

Πλοήγηση παιχνιδιού: steering, throttle, vessel inertia, sea state, bridge cameras, radar-like UI, route markers, berthing as simplified interactive gameplay, crew stations, fuel/resource abstraction, ship condition, repair, cargo/passengers, missions.

Να είναι εφικτό να περπατά ο παίκτης σε **επιλεγμένα playable τμήματα** του πλοίου, να μπαίνει στη γέφυρα και να αλλάζει ρόλο χωρίς να χαθεί ο κόσμος. Τα μεγάλα πλοία μπορούν αρχικά να λειτουργούν σε simplified simulation, με πλήρη playable interiors σταδιακά.

## 15. ΘΑΛΑΣΣΙΕΣ ΚΑΙ ΑΜΦΙΒΙΕΣ ΔΡΑΣΤΗΡΙΟΤΗΤΕΣ

Υποστήριξε θαλάσσιες μεταφορές, διάσωση, εξερεύνηση νησιών, ακτοπλοϊκό traffic, περιπολίες παιχνιδιού, αποστολές συνοδείας και σύνδεση πλοίων με λιμάνια, ελικόπτερα και οχήματα. Να υπάρχουν αφαιρετικές maritime encounters ως μέρος του campaign, χωρίς τεχνική αναπαραγωγή πραγματικών ναυτικών τακτικών.

Τα θαλάσσια συμβάντα ενημερώνουν το economy/world state όπως τα επίγεια και τα εναέρια.

## 16. COMBINED-ARMS EXPERIENCE

Ο στόχος είναι ο παίκτης να έχει **διαφορετικές οπτικές πάνω στο ίδιο γεγονός**:

- Ένας οικισμός χρειάζεται προμήθειες και μεταφορές.
- Η ίδια αποστολή μπορεί να προσεγγιστεί ως οδηγός convoy, πιλότος cargo plane, κυβερνήτης πλοίου ή πεζός διασώστης.
- Το world event και η ανταμοιβή είναι κοινά, αλλά αλλάζει ο τρόπος αλληλεπίδρασης.
- Μια μεγάλη scripted διαφωνία παρατάξεων επηρεάζει τοπικούς χάρτες, διαθεσιμότητα οχημάτων και NPC dialogues.

**Ο σχεδιασμός να επιβραβεύει διαφορετικούς ρόλους, όχι μόνο την καταστροφή.** Αποστολές διάσωσης, επιθεώρησης υποδομών και αποκατάστασης να είναι εξίσου σημαντικές.

## 17. ΣΥΣΤΗΜΑ NPC / ΤΕΧΝΗΤΗΣ ΝΟΗΜΟΣΥΝΗΣ

Χώρισε την AI σε:

1. Ambient civilian AI (κυκλοφορία, εργασία, καθημερινότητα).
2. Military/organization NPC AI (δραστηριότητες εντός του fictional world).
3. Vehicle driver/pilot AI (game-defined navigation).
4. Crew AI (job roles, dialogues, maintenance).
5. Strategic faction director (macro-simulation σε συγκεντρωτικό επίπεδο).
6. Narrative NPC AI (διαλογικές αντιδράσεις και σχέσεις).

Χρησιμοποίησε hierarchical FSM, Behavior Trees/Utility AI ή αντίστοιχες μηχανές για σύνθετες αποφάσεις. Οργάνωσε **LOD AI**: full κοντά, lightweight regionally, statistical remotely. Να υπάρχουν debug visualizers για NPC intent, active tasks, navmesh/path status και CPU cost.

Για σύγκρουση NPC, υλοποίησε game-level states και abstract resolution. Μην παράγεις ρεαλιστικά σχέδια επίθεσης/υπεράσπισης ή οδηγίες πραγματικής εμπλοκής.

## 18. SQUAD, CREW, COMMAND

Ο παίκτης μπορεί να συνεργάζεται με NPC και, αργότερα, να αναλαμβάνει ηγετικούς ρόλους.

- Simple squad orders: Follow, Stay, Regroup, Assist Objective, Return, Interact.
- Crew roles στα πλοία/αεροσκάφη/οχήματα.
- Morale, fatigue, readiness, trust και relationship ως abstract attributes.
- Availability, equipment compatibility και maintenance κατάσταση.
- Operation planner ως **game UI** με objectives, χρόνους, ρόλους NPC και χάρτη αποστολής, όχι πραγματικές οδηγίες επιχειρήσεων.

Το command mode είναι ένα **strategy layer με συμβολικές εντολές**. Η AI υλοποιεί αποφάσεις της με abstraction. Οι εντολές δεν χρειάζεται να μιμούνται πραγματικά στρατιωτικά εγχειρίδια.

## 19. ΠΑΡΑΤΑΞΕΙΣ ΚΑΙ ΠΟΛΙΤΙΚΗ

Δημιούργησε τουλάχιστον έξι πρωτότυπες φανταστικές ομάδες:

1. **Asterion Commonwealth**: ομοσπονδιακή διοίκηση και κοινωνικές υποδομές.
2. **Northshore Pact**: συνασπισμός με ιδιαίτερες περιφερειακές διεκδικήσεις.
3. **Cobalt Maritime League**: ναυτική και εμπορική ομοσπονδία.
4. **Orion Air & Logistics Consortium**: διεθνής οργανισμός μεταφορών και τεχνολογίας.
5. **Free Municipal Coalition**: ανεξάρτητες πολιτικές διοικήσεις.
6. **Relief & Mediation Network**: ουδέτερη οργάνωση ανθρωπιστικών αποστολών.

Κάθε παράταξη έχει: ID, όνομα, αξίες, περιοχές, διαθέσιμους πόρους, δείκτες σταθερότητας, βασικές σχέσεις, civilian support, reputation, δυναμική agenda και story flags.

Το πλαίσιο να επιτρέπει καινούριες παρατάξεις χωρίς αλλαγή του core code.

## 20. REPUTATION, TRUST ΚΑΙ CAREERS

Δύο επίπεδα φήμης:

- Global faction reputation.
- Local community reputation.

Κλίμακα -100 έως +100, με κατάσταση Hostile / Distrusted / Neutral / Trusted / Allied. Να επηρεάζει προσβάσιμες αποστολές, υπηρεσίες, οχήματα, διαθέσιμα crews, διάλογους και πολιτικές εξελίξεις.

Επιπλέον: certifications (fictional training missions), rank progression (game progression), trust και επιλογές ανεξαρτησίας. Το παιχνίδι να δίνει κίνητρα να αλλάξεις ειδικότητα χωρίς να χάνεις τον χαρακτήρα σου.

## 21. DYNAMIC CONFLICT MAP / FRONTLINES

Η σύγκρουση παρατάξεων είναι **μακρο-προσομοίωση** με αφαιρετικά variables ανά sector:

- Control index.
- Civic stability.
- Readiness.
- Supply availability.
- Public support.
- Infrastructure status.
- Environment/weather impact.
- Narrative pressure.

Τα sectors αλλάζουν κατάσταση από simulation ticks, world events και αποτελέσματα αποστολών. Η αλλαγή ελέγχου έχει άμεσες επιπτώσεις στους NPC, στα logistics, στις αποστολές, στο news feed και στις εμπορικές τιμές.

Οι συγκρούσεις δεν απαιτούν πάντα τον παίκτη. Τα εκτός οθόνης γεγονότα λύνονται **στατιστικά, με game-balanced probabilities** — χωρίς πραγματική στρατιωτική βελτιστοποίηση ή αυθεντικά δεδομένα επιχειρησιακής αποτελεσματικότητας.

## 22. ΔΥΝΑΜΙΚΗ ΟΙΚΟΝΟΜΙΑ / LOGISTICS

Η οικονομία παρακολουθεί αφαιρετικά:

- Food, water, fuel, medical goods, building materials, electronics, spare parts, civil transport capacity.
- Local stocks, production, consumption, prices, trade flows.
- Port/airfield/road service status.
- Supply chain interruptions.
- Repair requests, civilian demand και infrastructure restoration.

Οι οικονομικές αλλαγές δημιουργούν πραγματικές αποστολές και downstream αλλαγές στον κόσμο. Αποφύγετε να μοντελοποιείτε συγκεκριμένες πραγματικές στρατιωτικές αλυσίδες ανεφοδιασμού ως εκπαιδευτικό σύστημα.

## 23. ΠΟΛΙΤΕΣ ΚΑΙ ΑΝΘΡΩΠΙΝΟΣ ΚΟΣΜΟΣ

Ο κόσμος δεν αποτελεί μόνο μονάδες και χάρτες. Πρέπει να υπάρχουν:

- Civilian schedules και υπηρεσίες.
- Εμπόριο, εργασία, κυκλοφορία και δημοτικές υπηρεσίες.
- Σχολεία/νοσοκομεία ως κοινωνικές υποδομές, **όχι ως στόχοι μάχης**.
- Humanitarian missions και rebuilding quests.
- Δημόσια υποστήριξη και εμπιστοσύνη.
- Εναλλακτικές αποστολές διαλόγου/διαπραγμάτευσης.

Θέλω μη γραφική παρουσίαση συγκρούσεων, δυνατότητα αποφυγής άσκοπης βίας και επιπτώσεις στην κοινωνική σταθερότητα.

## 24. ΜISSION SYSTEM — MULTI-ROLE

Αποστολές:

- Main Story.
- Faction Career.
- Patrol/Observation (fictional tasks).
- Vehicle Transport.
- Rescue/Evacuation.
- Convoy Escort (abstract gameplay).
- Search & Survey.
- Infrastructure Repair.
- Civilian Relief.
- Air/Sea Transit.
- Recon Story Puzzle.
- Stealth Scenario.
- Fictional Combat Encounter.
- Investigation/Dialogue.
- Multi-stage Operations.

Κάθε mission είναι data-driven και διαθέτει ID, origin faction, region, role eligibility, prerequisites, objectives, branching choices, rewards, world effects, fail/retry rules και audit/event history.

Να υποστηρίζεται mission-specific role flexibility: ο ίδιος στόχος συχνά μπορεί να ολοκληρωθεί από διαφορετικά μέσα, χωρίς να εμφανίζονται ανεφάρμοστοι objectives.

## 25. DYNAMIC CAMPAIGN

Η campaign ιστορία να έχει τρεις βασικούς αφηγηματικούς άξονες:

**ACT I — Fractured Peace:** πολιτική κρίση, κρίση εμπιστοσύνης και καθημερινές αποστολές στον Asterion.

**ACT II — Shifting Lines:** μεταβολές ελέγχου περιοχών, τοπικές συγκρούσεις, διαταραχή υποδομών, σημαντικές αποφάσεις του παίκτη.

**ACT III — The Reconciliation Crisis:** πολλαπλά ενδεχόμενα για αποκλιμάκωση, αμοιβαία ασφάλεια, αποτυχία διαπραγμάτευσης ή νέα πολιτική ισορροπία.

Υποστήριξε ρόλους και μη μάχιμες λύσεις. Οι αποτυχίες αποστολών δεν πρέπει να προκαλούν αυθαίρετο hard lock: δώσε εναλλακτικές αποστολές, αντικρουόμενες προτεραιότητες και δυναμικά campaign branches.

## 26. EVENTS / NEWS / RADIO

Πρόσθεσε **World News Network** που αντλεί γεγονότα μόνο από πραγματικά world-state events:

- Αλλαγές σχέσεων παρατάξεων.
- Εξελίξεις υποδομών και εμπορίου.
- Μεταβολές ελέγχου περιοχών.
- Καιρικά φαινόμενα.
- Ανθρωπιστικές ανάγκες και επιτυχείς αποστολές.
- Εμφάνιση νέων προσώπων/αποστολών.

News ticker, radio chatter με πρωτότυπο, μη επιχειρησιακό διάλογο και ημερολόγιο γεγονότων. Να αποφεύγονται ψεύτικες ειδήσεις για πραγματικούς πολέμους ή πολιτικά πρόσωπα.

## 27. WEATHER, DAY/NIGHT, SEASONS ΚΑΙ ΦΥΣΙΚΟ ΠΕΡΙΒΑΛΛΟΝ

Συνέχισε σε ολόκληρο τον κόσμο με κοινό time system:

- Sunrise/sunset, κύκλος ημέρας-νύχτας.
- Rain, fog, wind, storm και θαλάσσια κατάσταση.
- Temperature/environment gameplay modifiers.
- Basic seasonal shifts.
- Dynamic soundscape, φωτισμός, βρεγμένες επιφάνειες.
- Mission variation ανά καιρό.

Ο καιρός να επηρεάζει **τη δυσκολία πλοήγησης και τη διαθεσιμότητα δραστηριοτήτων σε videogame επίπεδο**, όχι να καταλήγει σε εκπαιδευτική ανάλυση πραγματικών επιχειρησιακών ευπαθειών.

## 28. INTERIORS, BASES, HANGARS, PORTS

Δημιούργησε πυρήνες υψηλής λεπτομέρειας:

- Ένα μικρό κέντρο επιχειρήσεων.
- Μια πολιτική κωμόπολη.
- Ένα νοσοκομείο/κέντρο βοήθειας για mission interactions.
- Ένα αεροδρόμιο/hangar.
- Ένα λιμάνι.
- Μια βάση οχημάτων.
- Γέφυρα ενός πλοίου.
- Cockpit ενός αεροσκάφους/ελικοπτέρου.

Οι περιοχές να περιλαμβάνουν enter/exit, interactions, activity schedules, role assignment και persistent state. Πρώτα playable rooms, αργότερα περισσότερα interiors.

## 29. UPGRADE SYSTEM — ΜΕ ΝΟΗΜΑ

Συστήματα αναβάθμισης:

- Character clothing/suit/tool loadout.
- Inventory capacity.
- Vehicle handling, condition, payload gameplay.
- Helicopter/aircraft avionics ως simplified game categories.
- Naval navigation/crew/cargo slots.
- Crew skills και support technology.
- Hangars, repair facilities, non-combat base improvements.

Κάθε upgrade: ID, prerequisites, currency/material cost, compatibility tags, benefits, trade-offs, rarity, version. Απέφυγε να παρουσιάζεις όλες τις αναβαθμίσεις ως απλό linear power inflation.

## 30. PERSONNEL / FRIENDSHIPS / SQUAD STORIES

Πρόσθεσε NPC companions με skills, ονόματα, background, loyalty, availability, fatigue, personal storyline, contracts. Η αλλαγή ρόλου μπορεί να φέρει άλλους διαλόγους και οπτική της ίδιας κρίσης. Στο crew management να φαίνονται η θέση, η εμπειρία, οι ανάγκες και οι συνέπειες επιλογών.

## 31. COMMAND LAYER ΚΑΙ MANAGEMENT

Για ανώτερους ρόλους, συμπεριέλαβε υποσύστημα overview map με:

- Περιοχές, reputation και stability.
- Διαθέσιμα οχήματα και crews.
- Αποθέματα και κατάσταση υποδομών.
- Mission board.
- Abstract assignment slots (transport, maintenance, rescue, presence).
- Αποτελέσματα simulation ticks.
- News/events και πολιτικούς δείκτες.

Οι αποφάσεις είναι high-level οικονομικές, πολιτικές ή αποστολών. Δεν αποτελούν οδηγίες πραγματικού συντονισμού στρατιωτικών επιθέσεων.

## 32. UI / UX / HUD

Ενιαίο οπτικό σύστημα: muted military-style UI, ευανάγνωστα panel, διακριτές ενδείξεις κατάστασης, χάρτες, καθαρά icons, χωρίς υπερβολικά φορτωμένα overlays.

Απαραίτητα:

1. Main Menu / Profile.
2. New Campaign / Continue.
3. Character & Career Screen.
4. FPS/TPS HUD.
5. Vehicle HUD.
6. Helicopter Cockpit HUD.
7. Aircraft Cockpit HUD.
8. Naval Bridge & Map UI.
9. Tactical-style **game map** (όχι πραγματικό military planning).
10. World Status Map.
11. Factions & Reputation.
12. Mission Board & Journal.
13. Equipment / Inventory.
14. Vehicle Garage / Hangar / Dock.
15. Crew & Roles.
16. Shop / Economy.
17. News Feed.
18. Settings / Accessibility.
19. Save/Load.
20. Development Debug Overlay.

Όλα τα interfaces να συνδέονται με τα ίδια δεδομένα. Localization σε Ελληνικά/Αγγλικά, χωρισμένο από κώδικα.

## 33. GRAPHICS ENGINE ΚΑΙ VISUAL QUALITY

**Κύρια προτίμηση: Unreal Engine 5**, με C++/Blueprint υβριδική αρχιτεκτονική, World Partition/level streaming, δυνατότητες LOD/HLOD, PBR, animation blueprints, Niagara-like VFX και scalable graphics settings. Ζήτησε τεκμηριωμένη τελική επιλογή αφού ελέγξεις τη διαθεσιμότητα/εργαλεία του περιβάλλοντος και το πραγματικό hardware. Αν δεν είναι εφικτό να παραχθεί και να ελεγχθεί UE project, εξήγησε τους περιορισμούς και πρότεινε διακριτά το Godot 4.x ως ελαφρύτερη εναλλακτική. **Μη δημιουργήσεις δύο engines στο ίδιο development branch.**

Οπτικός στόχος: ρεαλιστική αλλά όχι υπερβολικά φωτορεαλιστική Γη, ατμοσφαιρικός φωτισμός, κινηματογραφικά vehicle exteriors, καθαρό cockpit, λεπτομερές terrain, καιρός, δέντρα, θάλασσα, πόλεις, NPC. Πρωτότυπα/αδειοδοτημένα assets, σωστά material budgets και όχι παράνομη εξαγωγή από υπάρχοντα παιχνίδια.

## 34. PHYSICS, INPUT ΚΑΙ GAME FEEL

Θέλω ενιαίο input abstraction για keyboard/mouse, gamepad, HOTAS, joystick όπου υποστηρίζεται. Profiles ανά role, rebindable actions, sensitivity/deadzone, camera FOV, inverted axes, assists, accessibility.

Physics layers:

- Character movement.
- Wheeled/tracked vehicle movement ως game physics.
- Rotor/fixed-wing flight dynamics ως game physics.
- Buoyancy/wave movement για πλοία.
- Collisions, object interactions, damage states.

Δεν ζητώ ακριβή εξομοίωση συγκεκριμένων πραγματικών πολεμικών μηχανών. Ο χειρισμός να είναι απολαυστικός, προβλέψιμος και δοκιμασμένος.

## 35. AUDIO, MUSIC ΚΑΙ CINEMATICS

- Adaptive original score ανά περιοχή/ένταση.
- Vehicle engine soundscape με sound layers.
- Ambient cities, forests, coast, hangars, interiors.
- Interaction cues και context dialogues.
- Story cutscenes μέσω engine timeline system.
- Clear subtitles και dynamic range settings.
- News / radio με procedural επιλογή εγκεκριμένων text snippets.

Μη χρησιμοποιείς copyrighted samples από παιχνίδια αναφοράς.

## 36. PERSISTENCE ΚΑΙ SAVE/LOAD

Τα saves πρέπει να περιλαμβάνουν player identity, θέση, role, current vehicle/seat, mission progress, world time, weather seed, NPC relationships, faction states, regions, economy, owned/assigned vehicles, inventory, crews, news history και event journal.

Απαίτηση: stable IDs, schema versions, migrations, snapshots, idempotent event application, ασφαλής αποθήκευση (atomic write), backup/rollback σε κατεστραμμένο save. Tests για αλλαγή role → save → load → σωστή ανάκτηση.

## 37. ΑΡΧΙΤΕΚΤΟΝΙΚΗ — ONE GAME, ONE TRUTH

Οργάνωσε modules:

- Core/Foundation
- WorldClock & Simulation Director
- Region & World Partition
- Entity Registry & Identity
- Player/Character
- Possession & Role/Seat Controller
- Interaction System
- Inventory & Equipment
- Vehicles (Ground)
- Aviation (Rotor/Fixed Wing)
- Naval
- AI/Navigation
- Factions/Reputation
- Economy/Infrastructure
- Mission & Campaign
- Crew/Squad
- Weather/Environment
- Audio/Visual Presentation
- UI
- Save/Load
- QA/Debug/Telemetry

Χρησιμοποίησε type-safe Event Bus ή αντίστοιχο προσεκτικά σχεδιασμένο message system, με ownership κανόνες. Κανένα module δεν πρέπει να γράφει απευθείας σε άσχετα mutable variables άλλου module. Οι world-state αλλαγές περνούν από σαφή commands/transactions και δημιουργούν audit events. Μη συγχέεις UI notifications με authoritative changes.

## 38. ΕΝΙΑΙΟ WORLD EVENT SYSTEM

Ελάχιστα event contracts:

- `RoleChanged`
- `VehicleEntered`
- `VehicleExited`
- `VehicleConditionChanged`
- `MissionAccepted`
- `MissionCompleted`
- `MissionFailed`
- `CommunityReputationChanged`
- `FactionReputationChanged`
- `RegionStabilityChanged`
- `RegionControlChanged`
- `InfrastructureStatusChanged`
- `ResourceStockChanged`
- `CivilianReliefDelivered`
- `WeatherChanged`
- `CampaignFlagChanged`
- `WorldNewsPublished`

Κάθε event να έχει `event_id`, `event_type`, `occurred_at_world_tick`, `source_entity_id`, `region_id`, `schema_version`, `payload`. Διπλό event δεν πρέπει να εφαρμόζεται δύο φορές. Τα subscribers να είναι deterministic όπου χρειάζεται.

## 39. ΠΑΡΑΔΕΙΓΜΑ ΠΡΑΓΜΑΤΙΚΗΣ ΣΥΝΔΕΣΗΣ ΣΥΣΤΗΜΑΤΩΝ

Μια φανταστική νησιωτική κοινότητα αντιμετωπίζει διακοπή εμπορικής σύνδεσης μετά από κακοκαιρία:

1. Το weather system καταγράφει μεγάλο καιρικό γεγονός.
2. Το port module μειώνει προσωρινά την operational availability.
3. Το economy system βλέπει χαμηλότερη ροή προμηθειών.
4. Η τοπική κοινότητα εμφανίζει αυξημένη ανάγκη για βασικά αγαθά.
5. Το mission generator εκδίδει αποστολές με θαλάσσια, εναέρια ή οδική μεταφορά όπου εφικτό.
6. Ο παίκτης επιλέγει ρόλο καπετάνιου ή πιλότου μεταφοράς και πραγματοποιεί αποστολή gameplay.
7. Τα παραδοθέντα resources ενημερώνουν το αποθεματικό του οικισμού.
8. Η τοπική reputation βελτιώνεται.
9. Αλλάζει ο διάλογος των NPC και το News Network καταγράφει το γεγονός.
10. Η καμπάνια μπορεί να χρησιμοποιήσει το νέο state σε επόμενο κεφάλαιο.

Αυτό είναι το πρότυπο κάθε μεγάλης δραστηριότητας. **Όχι απομονωμένα minigames με ψεύτικες συνέπειες.**

## 40. PERFORMANCE ΚΑΙ SIMULATION LOD

Επίπεδα:

- **A — Local Full Simulation:** full player, nearby NPC, 3D vehicles, interactions, physics.
- **B — Regional Lite Simulation:** compressed NPC intents, simplified travel και entity states.
- **C — Global Aggregate Simulation:** regions, factions, economy, policies, weather fronts ως statistics/events.

Fixed timestep για core simulation, render interpolation, streaming για assets, object pooling, asynchronous loading, HLOD, navigation streaming, deterministic test worlds. Προσδιόρισε συγκεκριμένα performance budgets μόλις είναι γνωστά target hardware/asset counts. Ο στόχος 60 FPS σε συμφωνημένο test rig δεν είναι ισχυρισμός ήδη επιτευγμένης απόδοσης.

## 41. DEFINITION OF PLAYABLE

Δεν αρκεί μια static scene ή character viewer. «Playable» σημαίνει:

- Εκκινεί η εφαρμογή και φορτώνει νέο/παλιό save.
- Ο παίκτης κινείται σε FPS/TPS.
- Μπαίνει και βγαίνει σε τουλάχιστον δύο διαφορετικές κατηγορίες μέσων.
- Υπάρχουν αλληλεπιδράσεις και ολοκληρώσιμες αποστολές.
- Ολοκλήρωση αποστολής αλλάζει authoritative world-state.
- Τα NPC και η οικονομία αντιδρούν και η αλλαγή επιβιώνει μετά το load.
- Δεν υπάρχουν κρυφά hardcoded «demo success» flags.
- Το UI, οι έλεγχοι και το game loop δεν απαιτούν χειροκίνητες παρεμβάσεις κώδικα.

## 42. FIRST VERTICAL SLICE — ASTERION ISLAND

Πρώτα φτιάξε ένα μικρό αλλά **πραγματικά συνδεδεμένο** ανοιχτό κομμάτι Γης:

- 1 συνεχής φανταστική νησιωτική περιοχή.
- 1 κωμόπολη με λειτουργικούς ambient NPC.
- 1 λιμάνι και 1 μικρό playable σκάφος.
- 1 αεροδιάδρομος και 1 playable ελικόπτερο.
- 1 βάση μεταφορών και 1 οδηγούμενο ground vehicle.
- 1 fictional armored vehicle ως αρχική δεύτερη ground κατηγορία.
- 1 playable character με FPS/TPS.
- 2 παρατάξεις + 1 ουδέτερος οργανισμός.
- 1 economy resource loop.
- 1 reputation loop.
- 3–5 αλληλένδετες αποστολές (διάσωση/μεταφορά/εξερεύνηση/story encounter).
- 1 abstract world-conflict event.
- 1 world news feed.
- 1 λειτουργικό save/load.

**First playable journey:** Ο παίκτης ξεκινά σε κωμόπολη, συνομιλεί με NPC, οδηγεί σε σημείο μεταφοράς, επιβιβάζεται σε ελικόπτερο ή πλοίο για μια αποστολή, φτάνει σε νέο σημείο του χάρτη, αποβιβάζεται, αλληλεπιδρά με objective, επιστρέφει, λαμβάνει ανταμοιβή, ενημερώνεται η φήμη και ο κόσμος, αποθηκεύει και φορτώνει σωστά.

## 43. ΠΛΗΡΕΣ DEVELOPMENT ROADMAP

**PHASE 0 — Architecture & Project Bootstrap:** επιλογή engine, version control, CI, folder structure, coding conventions, event contracts, save schema, sample map, test setup.

**PHASE 1 — Character + World:** FPS/TPS, interactions, world clock/weather stub, NPC ambient, camera, basic UI, save/load.

**PHASE 2 — Ground Vehicles:** driving, garages, seat/role transitions, camera/UI, condition/fuel abstraction, cargo.

**PHASE 3 — Aviation:** helicopter first, airfield, pilot seat, flight feel, basic AI traffic, transport missions; airplane after stable helicopter workflow.

**PHASE 4 — Naval:** vessel handling, sea/port streaming, bridge/camera, boarding, marine transport missions.

**PHASE 5 — Unified Missions & Progression:** mission engine, inventory, economy, reputation, roles, career unlocks, connected vertical slice.

**PHASE 6 — Living World:** faction simulation, region states, multi-rate LOD simulation, news, ambient activities, persistent consequences.

**PHASE 7 — Campaign / Advanced Roles:** multi-act narrative, companions, command UI, more vehicles, interiors, branches.

**PHASE 8 — Content / Polish / Release:** expanded terrain, optimizations, accessibility, audio, localization, bug fixes, packaging, save migration tests.

Κάθε φάση πρέπει να τελειώνει με acceptance criteria, automated smoke tests όπου εφικτό, changelog και περιγραφή known limitations. Προτιμώ ένα συμπαγές playable vertical slice από πολλά μισοτελειωμένα χαρακτηριστικά.

## 44. QA ΚΑΙ ACCEPTANCE TESTS

Ελάχιστες απαιτήσεις δοκιμών:

- `StartNewGame` δημιουργεί σωστό world state.
- `FPS_TPS_Toggle` δεν αλλάζει player identity.
- `EnterExitVehicle` διατηρεί θέση, inventory, mission.
- `RoleSwitchSaveLoad` διατηρεί seat και world state ή επαναφέρει safely fallback.
- `MissionCompletionOnce` δεν πληρώνει δύο φορές.
- `SupplyDeliveryChangesStock` ενημερώνει economy και news.
- `OffscreenWorldTick` εξελίσσει το simulation χωρίς ενεργό 3D rendering.
- `WorldEventReplay` δεν διπλοεφαρμόζει events.
- `FactionReputationVisibleInUI` εμφανίζει authoritative τιμή.
- `AircraftAndNavalInputProfiles` χρησιμοποιούν σωστό binding.
- `StreamingBoundary` δεν αφαιρεί οντότητες χωρίς σωστό persistence.

Κράτησε και manual playtest script με καθαρά expected outcomes.

## 45. DELIVERY CONTRACT

Σε κάθε ολοκληρωμένο milestone να παραδίδεις **πραγματικά αρχεία** της επιλεγμένης engine, πηγαίο κώδικα, περιεχόμενα asset folders ή νόμιμες αναφορές, configs, οδηγίες εκτέλεσης, changelog, test report και known issues.

- Μην κατασκευάζεις ψεύτικα download links, builds, EXE ή ZIP.
- Μην ισχυρίζεσαι «tested» αν δεν έτρεξαν tests.
- Μην αντικαθιστάς τον προηγούμενο κώδικα με νέο ασύνδετο prototype.
- Μην παραλείπεις ασύμβατες αλλαγές: εξήγησε migrations.
- Αν δεν έχεις πρόσβαση σε local editor/build tool, δώσε **ρητά** μη επαληθευμένο source και συγκεκριμένα local test steps.
- Διαχώρισε: IMPLEMENTED / PARTIAL / STUB / PLANNED / BLOCKED.

## 46. ΟΔΗΓΙΕΣ ΓΙΑ ΤΗΝ ΠΡΩΤΗ ΑΠΑΝΤΗΣΗ ΣΟΥ

Χωρίς να ξαναγράψεις απλώς αυτό το prompt, δώσε με τη σειρά:

**A. Executive Game Design Summary** με όραμα, διακριτά pillars και top gameplay loops.

**B. Technical Decision Record** για engine και συγκεκριμένο project structure, με σαφείς παραδοχές αν λείπει hardware spec.

**C. Architecture Diagram** που δείχνει world simulation, role switching, streaming, missions, economy, factions, AI, save και HUD.

**D. Data Models / Event Contracts** με παραδείγματα και ownership rules.

**E. Phase Plan** με εξαρτήσεις, acceptance criteria, βασικούς κινδύνους και προτεραιότητες.

**F. Vertical Slice Specification** για το Asterion Island, ακριβώς ποια χαρακτηριστικά χρειάζονται για μια πραγματικά playable διαδρομή.

**G. First Working Implementation**: ξεκίνα από την μικρότερη λειτουργική ενότητα Phase 0/1 που επιτρέπει ο εξοπλισμός και το περιβάλλον, με files, code και οδηγίες. Μην αφήσεις τη συζήτηση σε ατέρμονο brainstorming.

Αν μία λειτουργία είναι υπερβολικά ακριβή για το πρώτο build, παρουσίασε οικονομικό αντικατάστατο **που συνδέεται με τη μόνιμη αρχιτεκτονική**, χωρίς να ισχυριστείς ότι ολοκληρώθηκε η τελική μορφή.

## 47. ANTI-FAILURE RULES

- Όχι monolithic `GameManager` 20.000 γραμμών.
- Όχι κάθε όχημα ως ξεχωριστό παιχνίδι με ξεχωριστό HUD/inventory/time.
- Όχι UI που παρουσιάζει ψεύτικα world effects.
- Όχι campaign που προχωρά μόνο με cutscenes ενώ η δυναμική προσομοίωση αγνοείται.
- Όχι ασταθές random generation που αλλάζει μετά από load.
- Όχι endless world-scale χωρίς streaming.
- Όχι fully simulated remote NPC σε όλον τον χάρτη.
- Όχι αδιέξοδα mission states.
- Όχι έτοιμη φράση «όλα υλοποιήθηκαν» χωρίς αποδείξεις.
- Όχι αντιγραφή game assets, εμπορικών χαρτών ή protected IP.
- Όχι αληθινές στρατιωτικές οδηγίες ή απεικονίσεις ακραίας βίας.

## 48. ΤΕΛΙΚΗ ΕΜΠΕΙΡΙΑ ΠΟΥ ΘΕΛΩ

Θέλω **ένα τεράστιο ζωντανό παιχνίδι πάνω στη Γη**. Ξεκινώ ως ένας απλός χαρακτήρας σε ανοιχτό κόσμο. Μπορώ να δουλέψω ως οδηγός/χειριστής, να ενταχθώ σε φανταστική παράταξη, να γίνω πιλότος, να πετάξω ελικόπτερο ή αεροπλάνο, να χειριστώ τεθωρακισμένο, να κυβερνήσω πλοίο, να γυρίσω σε FPS/TPS, να συμμετέχω σε αποστολές, να εξερευνήσω και να εξελιχθώ σε ρόλο διοίκησης.

Εν τω μεταξύ οι πόλεις λειτουργούν, οι NPC ταξιδεύουν, οι κοινότητες αντιμετωπίζουν ανάγκες, οι παρατάξεις διαπραγματεύονται και συγκρούονται **σε αφαιρετικό επίπεδο παιχνιδιού**, οι υπηρεσίες επηρεάζονται από τον καιρό και οι αποστολές μεταβάλλουν μετρήσιμα την ιστορία.

**Πρόκειται για ένα ενιαίο παιχνίδι με κοινή προσομοίωση — όχι για συλλογή ασύνδετων minigames.** Φτιάξε σωστά τα θεμέλια και προχώρησε σε πραγματικά playable βήματα.
