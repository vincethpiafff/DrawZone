# Changelog DrawZone

Format inspiré de [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/).
Versions [SemVer](https://semver.org/lang/fr/) propres à DrawZone.

## [Non publié]

## [0.4.0] - 2026-10-28

Sortie vidéo de la vue de dessus (zones, personnes, slots) en Spout et NDI,
pour la voir dans un logiciel vidéo (Smode, Resolume, TouchDesigner, OBS…).

### Ajouté
- **Bloc Video Output** dans les paramètres du module Spatial : image propre
  de la vue de dessus, calculée par le module (marche Tracking Monitor
  fermé).
  - **Spout** (même machine ; demande « Use OpenGL Renderer ») et **NDI**
    (réseau ; demande le runtime NDI), actifs séparément ou ensemble, avec
    le même Stream Name et le même FPS. Rien n'est envoyé en NDI tant que
    personne ne reçoit.
  - **Output Area** : *Scene* (l'image = la scène, taille = scène × Pixels
    Per Meter, 100 px/m par défaut), *Custom Area* (Area Origin + Area Size
    en mètres), *Fixed Resolution* (toute la scène centrée dans Width ×
    Height). Pixel = (mètres − origine) × Pixels Per Meter ; taille limitée à
    8192 px ; champ Resolution en lecture seule (« 1000 x 600 px, 1 px = 1
    cm »).
  - **Display** : Grid, Zone Names, Person Ids, Trails, Slot Labels, Held
    Spots, Background (noir par défaut) et Transparent Background (alpha
    droit, NDI en BGRA). N'agit que sur l'image envoyée.
- Tracking Monitor : puces **Spout Output** et **NDI Output** dans l'en-tête
  (case = activer / désactiver, libellé = bloc Video Output dans
  l'Inspector).

### Technique
- Dessin de la vue de dessus extrait dans `ui/TopViewRenderer`, partagé par
  le Tracking Monitor et la sortie vidéo.
- Spout : instance propre de la bibliothèque Spout (`SendImage` sur le thread
  OpenGL de l'interface) ; l'émetteur partagé de Chataigne créait une
  seconde source noire.
- NDI : bibliothèque chargée comme le module « NDI out » de Chataigne
  (`VideoNetworkRuntime`), envoi sur un thread à part.

## [0.2.0] - 2026-10-28

Zones de toutes formes avec leurs métriques, et slots de joueurs stables
(« Joueur 1 / Joueur 2 »), le tout piloté depuis le Tracking Monitor.

### Ajouté
- **Formes de zone** : rectangle, ellipse (cercle si largeur = hauteur) et
  **forme libre** point par point, avec courbes (Linear / Quadratic / Cubic,
  poignées déplaçables). Dessin à la souris depuis le clic droit sur la
  scène : Draw rectangle / ellipse / free shape.
- **Métriques par zone** (Values) : Entered / Exited, Last Entered Id,
  Presence Time, Occupancy (avec Capacity), Average Position, Average
  Distance To Center.
- **Anti-clignotement** en bord de zone : Exit Margin (m) et Exit Delay (s).
- **Values > People** : une entrée par personne (Id, Position, Zone,
  Relative Position, Slot, Velocity, Orientation, Height, Age).
- **Slots de joueurs** (Player Slots) : chaque personne garde le même slot
  dans toute la scène. Deux modes « SwapSlots » : *Next Person Takes Slot*
  (slot libéré aussitôt pour la dernière personne arrivée) et *Hold Spot To
  Reclaim* (slot réservé là où la personne a été perdue ; la première
  personne sans slot qui entre dans le cercle le reprend ; Hold Time,
  Reclaim Radius). Slots renommables et colorés, sauvegardés avec la
  session ; par slot : Occupied, Person Id, Position, Held, Hold
  Remaining, Joined, Left.
- Tracking Monitor :
  - **cartes de zone** dépliables (occupation, temps, détail des valeurs) ;
    clic → Inspector avec les paramètres, les Values, puis les Points
    (repliés) ;
  - **bandeau Player Slots** sous la vue : puce du mode SwapSlots (menu
    expliqué), une puce par slot, boutons - / + ;
  - nom du slot qui suit chaque personne, cercle en pointillés avec le temps
    restant sur les emplacements réservés ;
  - **assignation de slot à la main** : glisser-déposer personne ↔ slot,
    touches 1–9 / 0 (pavé numérique aussi) sur la personne sélectionnée,
    clic droit → Assign slot / Release slot ; échange si la personne avait
    déjà un slot ;
  - liste **Shortcuts** dépliable dans l'en-tête ;
  - clic sur une personne → ses valeurs dans l'Inspector ;
  - traînée des 2 dernières secondes et flèche de direction des personnes ;
  - renommage par double clic, clic droit sur une zone → Duplicate, Delete,
    Hide, Set color... ; clic dans le vide → tout désélectionner.

### Modifié
- **Tracking Monitor** restylé (style sombre « show ») : barre d'état sur une
  ligne, zones occupées lumineuses, colonne Zones / People à droite
  (repliable en rail), boutons « + Create zone » et « Show hidden ».
- Le nombre de personnes n'est plus affiché en gros dans les zones de la vue
  de dessus (il reste sur les cartes).

## [0.1.0] - 2026-10-27

Premier « ça marche ! » : une personne trackée entre dans une zone, un
booléen bascule et pilote la State Machine de Chataigne.

### Ajouté
- Module **Spatial** (catégorie « DrawZone »), hérité du module OSC (D12) :
  réception Augmenta (`/object/enter|update|leave`, `/scene`) sur le port
  12000, liste de personnes indépendante du protocole, « Person Count ».
- Panneau **Tracking Monitor** : en-tête, vue de dessus de la scène (origine
  en haut à gauche, y vers le bas), tableau des personnes.
- **Zones rectangulaires** (centre et taille en mètres, bornées à la scène
  reçue) avec, par zone, « Presence » et « Person Count » dans Values,
  utilisables dans la State Machine et le Router ; sauvegardées avec la
  session.
- Zones dessinées et **éditables à la souris** dans le Tracking Monitor :
  déplacer, redimensionner (poignées), Ctrl+Z, Duplicate, Delete, Hide /
  « Show hidden zones », « Create zone » au clic droit.
- Documentation de transfert vers Claude Code : `CLAUDE.md`,
  `.github/README.md`, `docs/drawzone/` (BUILDING, ROADMAP, DECISIONS,
  CONTRIBUTING, JOURNAL, notes Augmenta).
- Script `Tools/drawzone/setup_windows.bat` : installe WebView2 et les DLL de
  dépendances après un clone.

### Modifié
- Aucun envoi vers les serveurs de Ben (mises à jour, rapports de plantage,
  statistiques, conversion en ligne) : bloc `// DrawZone:` dans
  `Source/Main.cpp`.

## [0.0.0] - 2026-10-23

### Ajouté
- Fork de Chataigne 1.10.5b6 (commit `35970109` de Ben).
- Projet Visual Studio 2022 régénéré pour le JUCE local (`D:\DRAWZONE\JUCE`).

### Validé
- Compilation Windows (Build Tools VS 2022, JUCE 7.0.12 de Ben).
- Réception OSC d'Augmenta Fusion dans Chataigne.
