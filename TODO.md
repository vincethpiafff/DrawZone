# TODO — DrawZone

Lire en début de séance avec `CLAUDE.md`. Fini : `docs/drawzone/TODO-archive.md`.
Légende : ✅ fait · ❌ échec / écarté (+ cause) · ⏭️ en cours · ⬜ à faire · ⏸️ reporté · 💬 Vincent · 💡 idée

**Écrire court** : une ligne par étape (date, quoi, résultat, chiffres du test).
Pas de récit, pas de répétition. Version terminée → archive, au mot près.

## ⏭️ En cours : v0.7.2 Logic Graph (v0.7.1 Live Mode publiée)

v0.6.0 publiée le 06/11/2026 (détail : `docs/drawzone/TODO-archive.md`, CHANGELOG). Contenu prévu (ROADMAP) :
événements, Branch, variables, sorties OSC.
- 💬 06/11 briques proposées : 1 moteur (nodes, broches, liens exécution / données, enregistré) · 2 éditeur « Logic Graph »
  (+ 💬 Vincent : **snapping des nodes**) · 3 événements · 4 données + Branch · 5 variables · 6 Send OSC · 7 test d'intégration.
- 💬 06/11 Vincent : brique **8, la dernière de la v0.7.2** : suivi de la condition, le chemin parcouru entre les nodes s'allume
  (glow + flèches) pour voir la condition testée et vraie. ⬜ proposition graphique à montrer avant de coder.
  💬 06/11 Vincent : bloc **Logic** du Cockpit, **plusieurs graphes nommés**, ordre **1 + 2** (testés ensemble), puis 3 à 6, puis 7.
- ✅ 06/11 4 rendus proposés (artifact « Logic Graph – 4 rendus ») : A Blueprint, B Cockpit, C Circuit, D Couloirs ;
  💬 Vincent : **B** (style du Tracking Monitor, aimantation sur les nodes, grille en option, mini-carte).
- ✅ briques 1 + 2, plan (branche `feature/logic-graph`) : 1a données `logic/LogicGraph.h/.cpp` (bloc Logic → graphes →
  nodes avec type et position, liens enregistrés) · 1b moteur au rythme des zones, 1ers nodes Zone Entered et Log · 2a panneau
  « Logic Graph » (onglets des graphes, déplacer / zoomer, créer par clic droit, glisser, relier, supprimer, annuler) · 2b
  aimantation sur les bords / centres des nodes + repères, grille 20 px en option, Alt = libre · 2c mini-carte.
- 💬 06/11 Vincent : **Live mode** (comme performance / On Air) avant le Logic Graph : coupe seulement l'affichage et l'édition
  (viewport gelé…) ; entrées tracking, zones, slots, Logic Graph, variables, OSC, PSN et Spout / NDI tournent toujours.
  Règle pour la brique 1 : le moteur du graphe tourne dans l'horloge du Cockpit, jamais dans le panneau. 
- 💬 06/11 Vincent : Live Mode = **v0.7.1**, Logic Graph = **v0.7.2**. Coupures retenues : viewport gelé, Logic Graph figé (avec la
  v0.7.2), Inspector gelé, statuts à 1 Hz, Logger réduit, édition verrouillée ; MCP en pause en option (désactivée par défaut).
- ✅ 06/11 v0.7.1 code (branche `feature/live-mode`) : bloc **Live Mode** du Cockpit (Enabled, Slow Preview, Pause MCP) ;
  Inspector : sélection coupée ; statuts 1 Hz ; Monitor : bouton LIVE, Ctrl + Shift + L, vue gelée ou 1 image / s, édition
  verrouillée. ❌ Logger réduit : pas de filtre dans le Logger d'organicUI (code de Ben) → écarté, à revoir avec Vincent.
  ❌ puis ✅ compilation : `setDescription` n'existe pas → `description`. Recompilation complète 0 erreur ; MCP : bloc présent,
  désactivé par défaut. Pas de simulation (pas de changement des données). ✅ Vincent : « ok commit et push », commit `e3f78409`.
- ✅ 06/11 v0.7.1 publiée : CHANGELOG, ROADMAP, README, CLAUDE.md ; fusion `--no-ff` dans `develop`, tag `v0.7.1`, push `origin`.
- ✅ 06/11 v0.7.2 briques 1 + 2, code : `logic/LogicGraph.h/.cpp` (bloc Logic → graphes → nodes, liens exec / données par uid,
  annulables ; moteur dans `updateZones` après zones, slots, personnes ; nodes Zone Entered et Log) ; `ui/LogicGraphPanel.h/.cpp`
  (style B : onglets + renommer / supprimer, fond à points, molette = zoom, clic droit glissé = déplacer la vue, clic droit =
  menus, glisser = déplacer, aimantation bords / centres + repères + étiquette, grille 20 px, Alt = libre, F = tout voir,
  Suppr, mini-carte, LIVE + Ctrl + Shift + L) ; sections Logic et Live dans l'Inspector ; panneau dans `MainComponent.cpp`
  (`// DrawZone:`) ; `.jucer` + Projucer. ❌ puis ✅ compilation : `LogicNode` abstrait (le gestionnaire crée des nodes vides)
  → catégorie par défaut. Compilé 0 erreur. Recompilation complète 0 erreur ; MCP : bloc Logic présent, 1 graphe. ✅ Vincent : « ok pour le logic graph ».
- ❌ 06/11 💬 Vincent : fenêtre réduite → en-tête coupé (sélecteur de surface introuvable en 2D). → en-têtes du Monitor et du
  Logic Graph en bande défilante : clic droit glissé ou molette, dégradé + flèche aux bords, clics limités à la partie visible,
  clic droit simple inchangé. Compilé 0 erreur. ✅ Vincent : « ok », commit `57b470ec` (avec les briques 1 + 2).
- ❌ 06/11 💬 Vincent : sens Y inversé entre 2D et 3D → écarté : Flip Y était coché sur sa source (pas un bug).
- ❌ 06/11 brique 3, 1re version (7 nodes, On / Off pour tout) : 💬 Vincent : Person Tracked n'est pas un booléen → règle par type de valeur.
- 💬 06/11 Vincent : nodes par type (Trigger → Then + Person Id ; Boolean → On / Off + State ; Integer → Changed + Count) ; temps en
  HH:MM:SS:FF (Frame Rate réglable, valeurs + réglages) ; pourcentage affiché en %. Dire « node ».
- ⏭️ 06/11 code : `SpatialUnits.h/.cpp` (Timecode, Percent ; `.jucer` + Projucer), Frame Rate du Cockpit, 6 temps + Occupancy convertis,
  broches Time / Percentage, 15 nodes (Events > Time / Zones / Slots / People), `PlayerSlots::events` pour Slot Joined / Left.
  Compilé 0 erreur, lien OK. `check_logic.ps1` réécrit (15 nodes, 77 puis 78 dans ZONE RECT) : simulation **40 / 40**. ✅ Vincent : liens conservés, commit `d1f28dff`.
- ⏭️ 06/11 💬 Vincent : menu du clic droit par type (Trigger, On-Off, Count, Time, Percentage, Meters, Outputs ; « / » = sous-menu → « On-Off ») ;
  8 nodes de données valeur + Changed (Zone Presence Time, Slot Hold Remaining, Person Age, Zone Occupancy, Person Position,
  Person Height, Slot Position, Zone Average Distance To Center), broche Meters. Compilé 0 erreur. ✅ Vincent : « ok tout bon ». Commit `a00994c9`.
- ⏭️ 06/11 brique 4 : node **Branch** (menu Flow) : In + Condition (On-Off ; réglage Condition si non reliée) → True / False. Compilé 0 erreur.
- 💬 06/11 Vincent : catégorie de nodes **Compare**, forme différente du rectangle. ⬜ 4 formes proposées (artifact « Logic Graph Compare Nodes » : A pastille, B hexagone, C losange, D rond) + liste (Compare, In Range, And, Or, Xor, Not), choix de Vincent. ✅ 💬 Vincent : **A pastille** ; sortie Result seule (On / Off plus tard si besoin). Code : catégorie Compare (Compare = ≠ > ≥ < ≤, In Range, And, Or, Xor, Not), dessinés en pastille (symbole + seuil) ; seuil dans l'unité de la valeur reliée (HH:MM:SS:FF, %, m). Compilé 0 erreur. ✅ Vincent : tests OK, commit `11fcb267`.
- 💬 06/11 Vincent : brique 5, nodes de **sortie vers des variables** typées : On-Off, pourcentage, temps HH:MM:SS:FF.
- 💬 06/11 Vincent (avant les tests) : clic droit glissé = **sélection au rectangle**, touche **H** = masquer / réafficher des nodes. ❌ 💬 Vincent : inverser les clics → clic **gauche** glissé dans le vide = rectangle (Ctrl / Maj = ajoute, clic simple = désélectionne), clic **droit** glissé = déplacer la vue, clic droit simple = menus, puis 💬 Vincent : clic droit simple = **seulement créer un node** (menus de node et de broche retirés : Suppr, H, et glisser un lien depuis son entrée pour le défaire), Ctrl / Maj + clic = ajoute / retire, déplacer / supprimer / H sur toute la sélection (une étape d'annulation), node masqué = Enabled décoché, estompé + « HIDDEN ». Compilé 0 erreur. ✅ Vincent : tests OK, commit `11fcb267`.
- ❌ 06/11 bug brique 1 : l'uid des nodes n'est pas enregistré (valeur par défaut) → liens perdus au rechargement. ✅ uid enregistré, copie = nouvel uid. Compilé 0 erreur.

- 💬 06/11 Vincent : positions relatives d'un tracker dans un slot, **zone et surface**. Code : nodes Person Relative Position, Slot Relative Position (Percentage : Zone X / Y, Surface X / Y, 0-1, -1 hors zone) et Slot Person (Count : id de la personne du slot, -1 libre). Compilé 0 erreur. + ValueNode : Changed se déclenche quand une valeur apparaît (slot pris, personne trackée). `check_logic.ps1` : 3 nodes ajoutés ; ❌ 43 / 45 : Surface X / Y attendus avec la Size manuelle (10 × 6) alors que SOL est en Auto (scène du simulateur 8 × 4.5) → test corrigé, code bon.
- 💬 06/11 Vincent : le réglage Slot des nodes = **liste des noms des slots** (Tracking Monitor), pas un numéro. Code : `LogicTargets::SlotSetting` (liste suit la surface : noms, nombre ; Any slot pour Joined / Left ; numéro caché enregistré sous /slot, anciens projets OK). Compilé 0 erreur ; ❌ exe sans ce code (build raté C1076, ModuleIncludes2/3 restés à 18:46, builds suivants « 0 erreur » à tort) → rebuild /m:3, textes neufs vérifiés dans l exe, simulation **45 / 45**. ✅ Vincent : tests OK.
- 💬 06/11 Vincent : **groupes de nodes** (cadre repliable / dépliable, couleur qui teinte les nodes dedans). 4 styles en artifact ; ✅ 💬 Vincent : **C fond plein**, + **pastille de couleur** dans le titre. Code : `LogicGroup` (bloc Groups du graphe ; nodes dont l'en-tête est dans le cadre, liste figée au repli), G = grouper la sélection, flèche ou double-clic sur le titre = replier (nodes cachés mais actifs, liens sortants = broches du groupe), titre = déplacer avec ses nodes, coin = redimensionner, pastille = sélecteur de couleur, Suppr = retire le groupe seul ; 💬 Vincent : **Ungroup** (menu Grouping + Maj + G, clic droit sur un titre = le sélectionne) : retire le cadre, garde les nodes. ✅ Vincent : tests OK. Compilé 0 erreur. ⬜ tests de Vincent.
- 💬 06/11 Vincent : **menu du clic droit** stylé, par parties (Grouping, Input Data, Logic, Output Data…). 4 styles en artifact (A sous-menus, B recherche, C deux colonnes, D tuiles) ; ✅ 💬 Vincent : **C deux colonnes**, + Group Selection / Empty Group dans le menu. Code : `LogicCreateMenu` (parties Grouping / Input Data / Logic / Output Data, survol = type, aide d'une ligne par node `LogicNodeManager::getDescription`), Échap ou clic à côté = ferme. Compilé 0 erreur. ❌ G sans effet chez Vincent : cause = exe périmé (build raté), G marche après le rebuild ; logs TEMP retirés. ✅ Vincent : tests OK.
- 💬 06/11 Vincent : brique 7 = **test en simulation de tous les nodes, fait par Claude**, avec beaucoup de cas de détection (long, à faire en entier).
- 💬 06/11 brique 5 : variables dans un **bloc Variables du Cockpit** (partagé, valeurs dans Values > Variables comme les zones) ; types On-Off, Number, Percentage, Time, Meters (Vincent n'a rien coché : les 5 par défaut, à confirmer). Idée : nodes par type (Set … en Outputs, Get … dans la catégorie du type ; Get On-Off = On / Off / State), valeur par défaut + Reset. ⏸️ reprise : tests de Vincent sur la 4b, commit, puis brique 5.
## ⬜ Questions ouvertes

- Choix de la carte réseau pour l'entrée PSN (la sortie l'a déjà).
- Kratos réel : signe de l'orientation non vérifié ; 2 émetteurs sur un même groupe
  PSN se mélangent (limite connue).

## ⬜ Plus tard

- **Script SMODE** `Tools/drawzone/smode/drawzone_osc_bank.py` (✅ écrit et testé hors
  SMODE, 28/10) : ⬜ test de Vincent dans SMODE après la v0.7 (sorties OSC du graphe).
  Non confirmé : types `Parameter(Real/Integer/String)`, `p.value.set` après l'append.
- Optimisation reportée : 17 (rendu vidéo sur un thread à part, si besoin mesuré),
  18 (copie unique de `TopViewModel`, à mesurer), 23 (Monitor limité au 1er Cockpit),
  27 (couper `TrackingMonitorPanel.cpp`, 2 300 lignes, en 4 fichiers).
- Inspector : barre d'icônes verticale (style C), après le style D.
- Réglages d'interface du Tracking Monitor : quand toutes ses briques seront finies.

## 💡 Idées

- 💡 26/10 Tracking Monitor plus « user friendly » (Vincent) : choix du module,
  vue et tableau redimensionnables, zoom, flèche de vitesse, colonnes au choix,
  survol personne ↔ ligne du tableau.
