# TODO — DrawZone

Lire en début de séance avec `CLAUDE.md`. Fini : `docs/drawzone/TODO-archive.md`.
Légende : ✅ fait · ❌ échec / écarté (+ cause) · ⏭️ en cours · ⬜ à faire · ⏸️ reporté · 💬 Vincent · 💡 idée

**Écrire court** : une ligne par étape (date, quoi, résultat, chiffres du test).
Pas de récit, pas de répétition. Version terminée → archive, au mot près.

## ⬜ Prochaine : v0.7, graphe nodal de base

v0.6.0 publiée le 06/11/2026 (détail : `docs/drawzone/TODO-archive.md`, CHANGELOG). Contenu prévu (ROADMAP) :
événements, Branch, variables, sorties OSC. ⬜ à découper en briques avec Vincent.

## ⬜ Questions ouvertes

- Choix de la carte réseau pour l'entrée PSN (la sortie l'a déjà).
- Kratos réel : signe de l'orientation non vérifié ; 2 émetteurs sur un même groupe
  PSN se mélangent (limite connue).

## ⬜ Plus tard

- **Script SMODE** `Tools/drawzone/smode/drawzone_osc_bank.py` (✅ écrit et testé hors
  SMODE, 28/10) : ⬜ test de Vincent dans SMODE après la v0.7 (sorties OSC du graphe).
  Non confirmé : types `Parameter(Real/Integer/String)`, `p.value.set` après l'append.
- Optimisation reportée : 17 (rendu vidéo sur un thread à part, si besoin mesuré),
  18 (copie unique de `TopViewModel`, à mesurer), 23 (Monitor limité au 1er Cockpit),
  27 (couper `TrackingMonitorPanel.cpp`, 2 300 lignes, en 4 fichiers).
- Inspector : barre d'icônes verticale (style C), après le style D.
- Réglages d'interface du Tracking Monitor : quand toutes ses briques seront finies.

## 💡 Idées

- 💡 26/10 Tracking Monitor plus « user friendly » (Vincent) : choix du module,
  vue et tableau redimensionnables, zoom, flèche de vitesse, colonnes au choix,
  survol personne ↔ ligne du tableau.
