# TODO — DrawZone

Lire en début de séance avec `CLAUDE.md`. Fini : `docs/drawzone/TODO-archive.md`.
Légende : ✅ fait · ❌ échec / écarté (+ cause) · ⏭️ en cours · ⬜ à faire · ⏸️ reporté · 💬 Vincent · 💡 idée

**Écrire court** : une ligne par étape (date, quoi, résultat, chiffres du test).
Pas de récit, pas de répétition. Version terminée → archive, au mot près.

## ⏭️ En cours : v0.6, viewport 3D

But (💬 31/10) : plusieurs scènes reçues de Fusion, placées en 3D par DrawZone ;
d'abord voir la scène en 3D, bascule 2D / 3D dans le Tracking Monitor, formes simples.

- ✅ 31/10 étude, sans code. Une seule source Augmenta reçoit l'OSC (un port par
  module) ; `/scene` v2 sans nom ; piste non vérifiée : un port par scène Fusion.
- Briques proposées : 1 vue 3D lecture seule (dessin JUCE du Monitor, sans OpenGL) ·
  2 source Augmenta avec son port · 3 placement 3D par scène · 4 formes simples ·
  5 test plusieurs scènes + 2 systèmes de tracking (ex. Fusion OSC + Naostage PSN).
- ⬜ Vincent : valider les briques ; zone liée à une scène (A, recommandé, utile pour
  un mur) ou commune au sol (B).
- ⬜ Vincent, avant la brique 2 : 2e scène Fusion, sortie OSC sur 12001 → capture.

## ⬜ Questions ouvertes

- Débit Augmenta de 213 msg/s avec 2 personnes (20 à vide), non expliqué.
- Choix de la carte réseau pour l'entrée PSN (la sortie l'a déjà).
- Kratos réel : signe de l'orientation non vérifié ; 2 émetteurs sur un même groupe
  PSN se mélangent (limite connue).
- Place du bloc Scene dans l'Inspector (entre Video Output et Zones) à confirmer.

## ⬜ Plus tard

- **Script SMODE** `Tools/drawzone/smode/drawzone_osc_bank.py` (✅ écrit et testé hors
  SMODE, 28/10) : ⬜ test de Vincent dans SMODE après la v0.7 (sorties OSC du graphe).
  Non confirmé : types `Parameter(Real/Integer/String)`, `p.value.set` après l'append.
- Optimisation reportée : 17 (rendu vidéo sur un thread à part, si besoin mesuré),
  18 (copie unique de `TopViewModel`, à mesurer), 23 (Monitor limité au 1er Cockpit).
- Inspector : barre d'icônes verticale (style C), après le style D.
- Réglages d'interface du Tracking Monitor : quand toutes ses briques seront finies.

## 💡 Idées

- 💡 26/10 Tracking Monitor plus « user friendly » (Vincent) : choix du module,
  vue et tableau redimensionnables, zoom, flèche de vitesse, colonnes au choix,
  survol personne ↔ ligne du tableau.
