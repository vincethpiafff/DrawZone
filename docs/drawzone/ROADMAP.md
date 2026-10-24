# Roadmap DrawZone

Principe : de petites briques, chacune compilable et testable avec Augmenta
Fusion. La 2D (zones au sol) passe avant la 3D : c'est 90 % des cas d'usage,
et elle donne un outil utilisable tôt.

| Version | Contenu | Résultat testable | État |
|---|---|---|---|
| **v0.0** | Outils installés, fork, compilation de Chataigne, réception d'Augmenta Fusion | Chataigne compilé en local qui reçoit Augmenta | ✅ 23/10/2026 |
| **v0.1** | Squelette du module Spatial : entrée OSC Augmenta, une zone rectangulaire | Un booléen « présence » qui bascule quand une personne simulée entre | ⏭️ prochaine |
| **v0.2** | Zones multiples (rectangle, cercle, polygone) et métriques | Valeurs utilisables dans la State Machine et le Router | |
| **v0.3** | Éditeur 2D vue de dessus | Dessiner et déplacer les zones à la souris, voir les personnes en direct | |
| **v0.4** | Entrées PSN et TUIO, multi-sources, transformation par source | Augmenta et un flux PSN dans la même scène | |
| **v0.5** | Viewport 3D | Surfaces, volumes, zones posées sur les surfaces | |
| **v0.6** | Graphe nodal de base | Événements, Branch, variables, sorties OSC | |
| **v0.7** | Graphe avancé | Timelines, Delay, DoOnce, machines à états, débogage visuel en direct, prefabs | |
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

À décider au démarrage : réutiliser le récepteur OSC de Chataigne (module
OSC existant) ou recevoir directement dans le module Spatial.

### v0.2 — métriques de zone

- Formes : rectangle, cercle, polygone.
- Par zone : présence, nombre de personnes, événements entrée/sortie,
  temps de présence, taux d'occupation (%), position normalisée dans la zone,
  distance au centre.
- Anti-clignotement en bord de zone (hystérésis, délai).

### v0.3 — éditeur 2D

- Panneau vue de dessus dans l'interface Chataigne (organicUI).
- Dessin et édition des zones à la souris.
- Affichage en direct des personnes trackées.

## Points durs connus

- Instabilité des identifiants : une personne perdue puis retrouvée change d'id.
- Bruit et gigue des positions (filtre de type One Euro à prévoir).
- Fusion multi-sources : ne compter qu'une fois une personne vue par deux systèmes.
- Robustesse en spectacle : mode show, redémarrage, état des sources.
- Viewport 3D : JUCE n'a qu'un contexte OpenGL (déprécié sur macOS).
