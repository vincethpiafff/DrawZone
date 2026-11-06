# Roadmap DrawZone

Principe : de petites briques, chacune compilable et testable avec Augmenta
Fusion. La 2D (zones au sol) passe avant la 3D : c'est 90 % des cas d'usage,
et elle donne un outil utilisable tôt.

Nom : le module « Spatial » s'affiche **Cockpit** depuis le 28/10/2026 (D14).

| Version | Contenu | Résultat testable | État |
|---|---|---|---|
| **v0.0** | Outils installés, fork, compilation de Chataigne, réception d'Augmenta Fusion | Chataigne compilé en local qui reçoit Augmenta | ✅ 23/10/2026 |
| **v0.1** | Squelette du module Spatial : entrée OSC Augmenta, une zone rectangulaire | Un booléen « présence » qui bascule quand une personne simulée entre | ✅ v0.1.0 (27/10/2026) |
| **v0.2** | Zones multiples (rectangle, cercle, polygone), métriques, slots de joueurs | Valeurs utilisables dans la State Machine et le Router ; « Joueur 1 / Joueur 2 » stables | ✅ v0.2.0 (28/10/2026) |
| **v0.3** | Éditeur 2D vue de dessus | Dessiner et déplacer les zones à la souris, voir les personnes en direct | 🟡 en grande partie fait en avance (Tracking Monitor) |
| **v0.4** | Sortie vidéo de la vue 2D en NDI et Spout | La vue de dessus (zones, personnes, slots) reçue dans un logiciel vidéo (Resolume, TouchDesigner, OBS…) | ✅ v0.4.0 (28/10/2026) |
| **v0.5** | Entrée PSN, multi-sources, transformation par source | Augmenta et un flux PSN dans la même scène | ✅ v0.5.0 (31/10/2026) |
| **v0.6** | Viewport 3D | Surfaces (sol, mur) placées en 3D, zones, slots et sortie vidéo par surface, PSN nommé par surface (D17) ; volumes : bonus | ✅ v0.6.0 (06/11/2026) |
| **v0.7.1** | Live Mode | Mode spectacle : affichage et édition coupés, tout le calcul et les sorties continuent | ✅ v0.7.1 (06/11/2026) |
| **v0.7.2** | Graphe nodal de base (Logic Graph) | Événements, Branch, variables, sorties OSC, éditeur avec aimantation (D18) | |
| **v0.8** | Graphe avancé | Timelines, Delay, DoOnce, machines à états, débogage visuel en direct, prefabs | |
| **v0.9** | Module TrackSimu (D16) | Blobs simulés dans Chataigne : une ou plusieurs scènes 2D sur des plans X / Y / Z, déplacés à la main ou par des mouvements (marche humaine, toucher d'une surface) | |
| **v1.0** | Stabilisation | Mode show, sauvegarde robuste, documentation | |
| **v2.x** | Capteurs bruts et calibration | LiDAR 2D/3D, caméras de profondeur (Orbbec…) | |

## Bonus (le jour où le cas réel se présente)

- Test de l'interface avec deux systèmes de tracking en même temps (ex. Fusion en OSC + Naostage en PSN), D16.
- Zones en volume, le jour d'un tracking 3D (ZacTrack, lidar 3D), D17.
- Slots par zone et autres règles d'attribution (la plus proche d'un point, la première entrée dans une zone), reportés en v0.2.

## Points durs connus

- Instabilité des identifiants : une personne perdue puis retrouvée change d'id.
- Bruit et gigue des positions (filtre de type One Euro à prévoir).
- Fusion multi-sources : ne compter qu'une fois une personne vue par deux systèmes.
- Robustesse en spectacle : mode show, redémarrage, état des sources.
- Viewport 3D : JUCE n'a qu'un contexte OpenGL (déprécié sur macOS).
