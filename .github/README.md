# DrawZone

**Dessiner des zones d'interaction, y brancher du tracking, en sortir de la logique.**

DrawZone est un outil open source pour les concepteurs d'expériences
immersives et interactives. Il reçoit des positions de personnes trackées
(Augmenta, Naostage, ZacTrack… via OSC, PosiStageNet), permet de
dessiner des zones sur des surfaces, d'y définir une logique inspirée du
game design (entrées/sorties de zone, conditions, timers, machines à états)
et d'envoyer le résultat en OSC : booléens, pourcentages, durées, compteurs.

> **Statut : en développement (v0.5.0 publiée, v0.6 en cours : surfaces en 3D).** Pas encore prêt pour la production.

## Basé sur Chataigne

DrawZone est un fork de [Chataigne](https://github.com/benkuper/Chataigne),
de Ben Kuper : « Artist-friendly Modular Machine for Art and Technology ».
Il en reprend l'interface, les protocoles et les outils (State Machine,
Module Router, Parrot, Dashboard), et y ajoute un module spatial dédié au
tracking. Merci à Ben Kuper et aux contributeurs de Chataigne.

La présentation d'origine de Chataigne est dans le [README racine](../README.md).

## Feuille de route

Versions, contenu et état : [docs/drawzone/ROADMAP.md](../docs/drawzone/ROADMAP.md)

## Compiler

Windows uniquement pour l'instant : [docs/drawzone/BUILDING.md](../docs/drawzone/BUILDING.md)

## Documentation du projet

- [Décisions](../docs/drawzone/DECISIONS.md)
- [Conventions de travail](../docs/drawzone/CONTRIBUTING.md)
- [Changelog](../docs/drawzone/CHANGELOG.md)

## Licence

GPL-3.0, comme Chataigne. Voir [LICENSE](../LICENSE).
