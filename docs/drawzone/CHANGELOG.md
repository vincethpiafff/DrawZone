# Changelog DrawZone

Format inspiré de [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/).
Versions [SemVer](https://semver.org/lang/fr/) propres à DrawZone.

## [Non publié]

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
