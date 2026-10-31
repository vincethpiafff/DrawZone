# Roadmap DrawZone

Principe : de petites briques, chacune compilable et testable avec Augmenta
Fusion. La 2D (zones au sol) passe avant la 3D : c'est 90 % des cas d'usage,
et elle donne un outil utilisable tôt.

Nom : le module « Spatial » s'affiche **Cockpit** depuis le 28/10/2026 (D14).

| Version | Contenu | Résultat testable | État |
|---|---|---|---|
| **v0.0** | Outils installés, fork, compilation de Chataigne, réception d'Augmenta Fusion | Chataigne compilé en local qui reçoit Augmenta | ✅ 23/10/2026 |
| **v0.1** | Squelette du module Spatial : entrée OSC Augmenta, une zone rectangulaire | Un booléen « présence » qui bascule quand une personne simulée entre | ✅ v0.1.0 (27/10/2026) |
| **v0.2** | Zones multiples (rectangle, cercle, polygone), métriques, slots de joueurs | Valeurs utilisables dans la State Machine et le Router ; « Joueur 1 / Joueur 2 » stables | |
| **v0.3** | Éditeur 2D vue de dessus | Dessiner et déplacer les zones à la souris, voir les personnes en direct | 🟡 en grande partie fait en avance (Tracking Monitor) |
| **v0.4** | Sortie vidéo de la vue 2D en NDI et Spout | La vue de dessus (zones, personnes, slots) reçue dans un logiciel vidéo (Resolume, TouchDesigner, OBS…) | ✅ v0.4.0 (28/10/2026) |
| **v0.5** | Entrée PSN, multi-sources, transformation par source | Augmenta et un flux PSN dans la même scène | ✅ v0.5.0 (31/10/2026) |
| **v0.6** | Viewport 3D | Surfaces, volumes, zones posées sur les surfaces | ⬜ avec, en même temps, le test de l'interface avec plusieurs scènes et plusieurs systèmes de tracking en entrée (ex. Fusion en OSC + Naostage en PSN) |
| **v0.7** | Graphe nodal de base | Événements, Branch, variables, sorties OSC | |
| **v0.8** | Graphe avancé | Timelines, Delay, DoOnce, machines à états, débogage visuel en direct, prefabs | |
| **v1.0** | Stabilisation | Mode show, sauvegarde robuste, documentation | |
| **v2.x** | Capteurs bruts et calibration | LiDAR 2D/3D, caméras de profondeur (Orbbec…) | |

## Détail des prochaines versions

### v0.1 — premier « ça marche ! »

- Nouveau type de module « Spatial » dans la catégorie de modules de Chataigne.
- Réception des messages Augmenta (`/object/update`, `/object/leave`, `/scene`).
- Liste des personnes suivies (id, position) dans le module.
- Une zone rectangulaire définie par des paramètres (centre, taille).
- Un paramètre booléen « présence », visible dans l'Inspector et utilisable
  dans la State Machine.

Décidé au démarrage (D12, 24/10/2026) : le module Spatial hérite du module
OSC de Chataigne.

### v0.2 — métriques de zone

- Formes : rectangle, cercle, polygone.
- Par zone : présence, nombre de personnes, événements entrée/sortie,
  temps de présence, taux d'occupation (%), position normalisée dans la zone,
  distance au centre.
- Anti-clignotement en bord de zone (hystérésis, délai).

**Slots de joueurs** (idée de Vincent, 24/10/2026), après les métriques :

- L'utilisateur fixe un nombre de slots (ex. 2 : « Joueur 1 », « Joueur 2 »),
  quel que soit le nombre de personnes détectées, pour tout le module ou
  pour une zone.
- Une règle d'attribution choisie dans une liste. Première règle : la
  première personne détectée prend et conserve le slot 1, la deuxième le
  slot 2. Autres règles possibles plus tard : la plus proche d'un point,
  la première entrée dans une zone.
- Une règle de libération : à la sortie, ou après un délai de grâce (une
  personne perdue puis retrouvée sous un nouvel id, près du même endroit,
  récupère son slot). Lié au point dur « instabilité des identifiants ».
- Chaque slot expose ses paramètres (occupé, position, id Augmenta),
  utilisables dans la State Machine comme les métriques de zone.

Fait en v0.2.0 : slots pour toute la scène, deux modes « SwapSlots » (Next
Person Takes Slot, Hold Spot To Reclaim), assignation à la main depuis le
Tracking Monitor. Reportés : slots par zone et autres règles d'attribution
(la plus proche d'un point, la première entrée dans une zone).

### v0.3 — éditeur 2D

- Panneau vue de dessus dans l'interface Chataigne (organicUI).
- Dessin et édition des zones à la souris.
- Affichage en direct des personnes trackées.

### v0.4 — sortie vidéo NDI / Spout (décidé le 28/10/2026)

- La vue de dessus du Tracking Monitor (zones, personnes, slots, emplacements
  réservés) envoyée en flux vidéo : **Spout** (Windows, même machine) et
  **NDI** (réseau).
- Utilisable dans un logiciel vidéo (Resolume, TouchDesigner, OBS…) comme
  retour visuel ou comme source d'un mapping.
- ✅ Fait en v0.4.0 : bloc Video Output (Spout, NDI, Output Area, FPS,
  options d'affichage, fond transparent) et puces dans l'en-tête du
  Tracking Monitor. Détail dans le CHANGELOG.

Les versions suivantes sont décalées d'un cran : PSN / TUIO en v0.5,
viewport 3D en v0.6, graphe nodal en v0.7 et v0.8.

## Points durs connus

- Instabilité des identifiants : une personne perdue puis retrouvée change d'id.
- Bruit et gigue des positions (filtre de type One Euro à prévoir).
- Fusion multi-sources : ne compter qu'une fois une personne vue par deux systèmes.
- Robustesse en spectacle : mode show, redémarrage, état des sources.
- Viewport 3D : JUCE n'a qu'un contexte OpenGL (déprécié sur macOS).
