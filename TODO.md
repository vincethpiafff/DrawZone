# TODO — DrawZone

Lire en début de séance avec `CLAUDE.md`. Fini : `docs/drawzone/TODO-archive.md`.
Légende : ✅ fait · ❌ échec / écarté (+ cause) · ⏭️ en cours · ⬜ à faire · ⏸️ reporté · 💬 Vincent · 💡 idée

**Écrire court** : une ligne par étape (date, quoi, résultat, chiffres du test).
Pas de récit, pas de répétition. Version terminée → archive, au mot près.

## ⏭️ En cours : v0.7.2 Logic Graph (v0.7.1 Live Mode publiée)

v0.6.0 publiée le 06/11/2026 (détail : `docs/drawzone/TODO-archive.md`, CHANGELOG). Contenu prévu (ROADMAP) :
événements, Branch, variables, sorties OSC.
- 💬 06/11 briques proposées : 1 moteur (nœuds, broches, liens exécution / données, enregistré) · 2 éditeur « Logic Graph »
  (+ 💬 Vincent : **snapping des nœuds**) · 3 événements · 4 données + Branch · 5 variables · 6 Send OSC · 7 test d'intégration.
  💬 06/11 Vincent : bloc **Logic** du Cockpit, **plusieurs graphes nommés**, ordre **1 + 2** (testés ensemble), puis 3 à 6, puis 7.
- ✅ 06/11 4 rendus proposés (artifact « Logic Graph – 4 rendus ») : A Blueprint, B Cockpit, C Circuit, D Couloirs ;
  💬 Vincent : **B** (style du Tracking Monitor, aimantation sur les nœuds, grille en option, mini-carte).
- ⬜ briques 1 + 2, plan (branche `feature/logic-graph`) : 1a données `logic/LogicGraph.h/.cpp` (bloc Logic → graphes →
  nœuds avec type et position, liens enregistrés) · 1b moteur au rythme des zones, 1ers nœuds Zone Entered et Log · 2a panneau
  « Logic Graph » (onglets des graphes, déplacer / zoomer, créer par clic droit, glisser, relier, supprimer, annuler) · 2b
  aimantation sur les bords / centres des nœuds + repères, grille 20 px en option, Alt = libre · 2c mini-carte.
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
