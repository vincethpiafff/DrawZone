# Notes : protocole Augmenta

État : **observations seulement**. À compléter avec la spécification
officielle avant d'écrire le décodeur (début de v0.1).

## Sources à lire

- Wiki Augmenta : https://github.com/Augmenta-tech/Augmenta/wiki
  (page « Data » pour le protocole).
- Module Augmenta pour Chataigne (navigateur de modules de Chataigne,
  « Get more modules ») : bonne référence de décodage.
- Documentation actuelle : https://docs.augmenta.tech/

## Observé le 23/10/2026 (Augmenta Fusion → Chataigne, OSC, port 12000)

Fréquence : environ 10 messages `/scene` par seconde.

| Adresse | Exemple d'arguments | Interprétation probable (à confirmer) |
|---|---|---|
| `/scene` | `837 2 8 4.5` | n° d'image, nombre de personnes, largeur (m), hauteur (m) |
| `/fusion` | `0 0 8 4.5 1920 1080` | décalage x, y, taille (m), résolution en pixels |
| `/object/leave` | `840 6 2 14.059 0.793945 0.451389 9.78913e-13 …` | n° d'image, id, index, âge (s), centroïde x, y normalisés, vitesse… |
| `/object/leave/extra` | `840 6 2 0 0 0 0` | données complémentaires de l'objet |
| `/object/update` | (non relevé en détail) | mise à jour de position, même structure que `/object/leave` |

Scène simulée : 8 × 4,5 m, 1920 × 1080 px.

## Questions à trancher en v0.1

- Version exacte du protocole envoyée par Fusion (Augmenta v2 « /object/… »
  ou une version plus récente) et liste des arguments.
- Repère : origine, sens des axes, coordonnées normalisées (0–1) ou en mètres.
- Gestion de `/object/enter` (non observé dans l'extrait).
- Réutiliser le module OSC de Chataigne comme récepteur, ou ouvrir un port
  directement dans le module Spatial.
