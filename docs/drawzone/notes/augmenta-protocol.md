# Notes : protocole Augmenta

État : **protocole OSC v2 recoupé** (24/10/2026) entre la doc Augmenta, le
code du plugin officiel Unreal et nos observations du 23/10.
**26/10/2026** : `/object/update` décodé par le module Spatial (id,
centroïde) et le sens des axes vérifiés en direct dans le Tracking Monitor.
Reste à vérifier : `/object/enter`.

## Sources

- Wiki Augmenta, page « Data » : https://github.com/Augmenta-tech/Augmenta/wiki/Data
  Elle renvoie désormais vers https://docs.augmenta.tech, où nous n'avons pas
  retrouvé la page du protocole. Extrait encore visible dans les moteurs de
  recherche : « Augmenta OSC Protocol v2.0 is the current default one » et
  « All coordinates are (otherwise specified) normalized [0-1] with a top left
  origin ».
- Plugin officiel Unreal, code de décodage :
  https://github.com/Augmenta-tech/AugmentaUnreal
  (`Source/AugmentaUnreal/Private/AugmentaReceiver.cpp`). C'est la référence
  utilisée pour l'ordre des arguments ci-dessous.
- Module Augmenta pour Chataigne (navigateur de modules de Chataigne,
  « Get more modules ») : non lu.

## Protocole OSC v2 (port 12000 par défaut)

### `/scene`

| Index | Champ | Type |
|---|---|---|
| 0 | n° d'image (frame) | int |
| 1 | nombre de personnes | int |
| 2 | largeur de la scène (m) | float |
| 3 | hauteur de la scène (m) | float |

### `/object/enter`, `/object/update`, `/object/leave`

Même structure pour les trois. Le plugin Unreal ne lit que l'index 1 pour
`/object/leave`.

| Index | Champ | Type | Remarque |
|---|---|---|---|
| 0 | frame | int | |
| 1 | **id** | int | identifiant unique de la personne : c'est la clé à utiliser |
| 2 | oid | int | index de la personne dans la liste courante (0, 1, 2…) : change |
| 3 | âge | float | secondes depuis l'entrée |
| 4 | centroïde x | float | normalisé 0–1 |
| 5 | centroïde y | float | normalisé 0–1 |
| 6 | vitesse x | float | |
| 7 | vitesse y | float | |
| 8 | orientation | float | |
| 9 | rectangle englobant, position x | float | |
| 10 | rectangle englobant, position y | float | |
| 11 | rectangle englobant, largeur | float | |
| 12 | rectangle englobant, hauteur | float | |
| 13 | rectangle englobant, rotation | float | |
| 14 | hauteur de la personne | float | |

**Position en mètres** = centroïde normalisé × taille de la scène (`/scene`
index 2 et 3). Origine en haut à gauche, d'après la doc.

### Données « extra »

Fusion envoie `/object/leave/extra` (observé). Le plugin Unreal attend
`/object/extra/enter|update|leave`. Les deux formes existent selon les
versions : à gérer toutes les deux si on en a besoin (pas en v0.1).

| Index | Champ |
|---|---|
| 0 | frame |
| 1 | id |
| 2 | oid |
| 3, 4 | point le plus haut x, y |
| 5 | distance |
| 6 | réflectivité |

### `/fusion`

`0 0 8 4.5 1920 1080` : décalage x, y, taille (m), résolution en pixels.
Non décodé par le plugin Unreal, inutile pour la v0.1.

## Observé le 23/10/2026 (Augmenta Fusion → Chataigne, OSC, port 12000)

Fréquence : environ 10 messages `/scene` par seconde. Scène simulée :
8 × 4,5 m, 1920 × 1080 px.

| Adresse | Exemple d'arguments | Lecture avec le tableau ci-dessus |
|---|---|---|
| `/scene` | `837 2 8 4.5` | frame 837, 2 personnes, 8 × 4,5 m |
| `/fusion` | `0 0 8 4.5 1920 1080` | voir ci-dessus |
| `/object/leave` | `840 6 2 14.059 0.793945 0.451389 9.78913e-13 …` | frame 840, id 6, oid 2, âge 14,06 s, centroïde (0,79 ; 0,45) → (6,35 m ; 2,03 m), vitesse ≈ 0 |
| `/object/leave/extra` | `840 6 2 0 0 0 0` | données extra vides |

## Points à vérifier en direct (v0.1)

- Fusion envoie-t-il bien `/object/enter` ? (non observé le 23/10)
- ✅ Sens des axes (26/10/2026, Vincent, personne déplacée dans Fusion et
  suivie dans la vue de dessus du Tracking Monitor) : **origine en haut à
  gauche, x vers la droite, y vers le bas**, conforme à la doc.
- Une personne perdue puis retrouvée change-t-elle d'id ? (voir « Points
  durs » de la ROADMAP)
