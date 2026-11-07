# TODO — DrawZone

Lire en début de séance avec `CLAUDE.md`. Fini : `docs/drawzone/TODO-archive.md`.
Légende : ✅ fait · ❌ échec / écarté (+ cause) · ⏭️ en cours · ⬜ à faire · ⏸️ reporté · 💬 Vincent · 💡 idée

**Écrire court** : une ligne par étape (date, quoi, résultat, chiffres du test).
Pas de récit, pas de répétition. Version terminée → archive, au mot près.

## ⏭️ Suite : v0.8 Graphe avancé (v0.7.2 Logic Graph publiée le 07/11/2026)

v0.7.1 et v0.7.2 : `docs/drawzone/TODO-archive.md`, CHANGELOG. Prévu (ROADMAP) : Timelines, Delay, DoOnce, machines à états,
débogage visuel en direct (+ illumination du chemin, bonus mis de côté le 07/11, voir Plus tard), prefabs. ⬜ à découper avec Vincent.

## ⬜ Questions ouvertes

- Choix de la carte réseau pour l'entrée PSN (la sortie l'a déjà).
- Kratos réel : signe de l'orientation non vérifié ; 2 émetteurs sur un même groupe
  PSN se mélangent (limite connue).

## ⬜ Plus tard

- **Bonus Logic Graph : illumination du chemin** (ex-brique 8 de la v0.7.2, mise de côté le 07/11) : lien parcouru et nodes atteints qui s allument. 3 styles proposés (artifact « Logic Graph Path Glow » : A lueur qui s éteint, B impulsion qui voyage, C traînée + valeurs des liens de données), interrupteur Show flow. Choix de Vincent à faire.
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
