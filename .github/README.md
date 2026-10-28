# DrawZone

**Dessiner des zones d'interaction, y brancher du tracking, en sortir de la logique.**

DrawZone est un outil open source pour les concepteurs d'expériences
immersives et interactives. Il reçoit des positions de personnes trackées
(Augmenta, Naostage, ZacTrack… via OSC, PosiStageNet, TUIO), permet de
dessiner des zones sur des surfaces, d'y définir une logique inspirée du
game design (entrées/sorties de zone, conditions, timers, machines à états)
et d'envoyer le résultat en OSC : booléens, pourcentages, durées, compteurs.

> **Statut : en développement précoce (v0.0.0).** Pas encore utilisable en production.

## Basé sur Chataigne

DrawZone est un fork de [Chataigne](https://github.com/benkuper/Chataigne),
de Ben Kuper : « Artist-friendly Modular Machine for Art and Technology ».
Il en reprend l'interface, les protocoles et les outils (State Machine,
Module Router, Parrot, Dashboard), et y ajoute un module spatial dédié au
tracking. Merci à Ben Kuper et aux contributeurs de Chataigne.

La présentation d'origine de Chataigne est dans le [README racine](../README.md).

## Feuille de route

| Version | Contenu |
|---|---|
| v0.1 | Module Spatial : entrée Augmenta, une zone, booléen de présence |
| v0.2 | Zones multiples et métriques (nombre, durée, occupation…) |
| v0.3 | Éditeur 2D vue de dessus |
| v0.4 | Sortie vidéo de la vue 2D en NDI et Spout |
| v0.5 | Entrées PosiStageNet et TUIO, multi-sources |
| v0.6 | Viewport 3D |
| v0.7–0.8 | Éditeur nodal façon Blueprints |
| v1.0 | Version stable |

Détails : [docs/drawzone/ROADMAP.md](../docs/drawzone/ROADMAP.md)

## Compiler

Windows uniquement pour l'instant : [docs/drawzone/BUILDING.md](../docs/drawzone/BUILDING.md)

## Documentation du projet

- [Décisions](../docs/drawzone/DECISIONS.md)
- [Conventions de travail](../docs/drawzone/CONTRIBUTING.md)
- [Changelog](../docs/drawzone/CHANGELOG.md)
- [Journal de bord](../docs/drawzone/JOURNAL.md)

## Licence

GPL-3.0, comme Chataigne. Voir [LICENSE](../LICENSE).
