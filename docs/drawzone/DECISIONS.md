# Décisions

Une décision, sa raison, sa date. Ne pas la remettre en cause sans motif.
Ajouter en bas, en 2 ou 3 lignes. Numéros de version : ceux de l'époque (voir D13).

- **D1 (23/10)** Fork de Chataigne plutôt que de zéro : Vincent connaît l'interface ;
  protocoles (OSC, MIDI, DMX, PSN…), State Machine, Router, Parrot, Dashboard déjà faits.
  Conséquence : C++/JUCE, GPL-3.0 ; contacter Ben une fois une base fonctionnelle.
- **D2 (23/10)** Zones = un module Chataigne qui expose des paramètres : tout
  Chataigne (State Machine, Router, Timeline, Parrot) marche sans réécriture.
- **D3 (23/10)** À terme, éditeur nodal façon Blueprints d'Unreal, en plus de la
  State Machine : familier aux créatifs du jeu vidéo.
- **D4 (23/10)** Windows seul (Build Tools VS 2022) au début ; Mac et Linux plus tard.
- **D5 (23/10)** 2D avant 3D : la plupart des zones sont au sol, outil utile plus tôt.
- **D6 (23/10)** Tests avec Augmenta Fusion (possédé par Vincent).
- **D7 (23/10)** JUCE à côté du dépôt, jamais dedans (règle de Ben, `AGENTS.md`).
- **D8 (23/10)** Projucer compilé avec `JUCER_ENABLE_GPL_MODE=1` : sinon il refuse
  d'enregistrer sans splash screen. Prévu par JUCE pour les projets GPL.
- **D9 (23/10)** `core.autocrlf=false` : sinon chaque `--resave` change ≈ 1000 lignes
  de `BinaryData` et `JuceLibraryCode`.
- **D10 (23/10)** Divergence minimale avec Ben (nos fichiers, accroches `// DrawZone:`,
  `README.md` racine de Ben, le nôtre dans `.github/README.md`) : fusions faciles.
- **D11 (23/10)** `Chataigne_App.vcxproj` commité avec notre chemin JUCE (`3d851a53`,
  `v0.0.0`) : un clone compile directement. Conflit : version de Ben puis `--resave`.
- **D12 (24/10)** `SpatialModule` hérite de `OSCModule` (comme ses modules PowerPoint,
  Millumin) et traduit l'OSC en liste de personnes indépendante du protocole :
  port, carte réseau, logs, sorties OSC gratuits ; PSN = autre traducteur.
  Conséquence : deux modules ne partagent pas un port.
- **D13 (28/10)** Sortie vidéo NDI / Spout de la vue 2D en v0.4 ; PSN en v0.5, 3D en v0.6,
  nodal en v0.7–v0.8. Choix de Vincent : utile tout de suite en spectacle.
- **D14 (28/10)** Nom visible **Cockpit** (« aux manettes d'un vaisseau spatial »),
  code **Spatial** (renommer le code coûterait pour rien). `ModuleFactory::create`
  charge un ancien type « Spatial » comme Cockpit.
- **D15 (30/10)** Pas d'entrée TUIO (choix de Vincent) : Augmenta (OSC) et PSN seulement.
- **D16 (05/11)** Test avec deux systèmes de tracking à la fois : reporté en bonus,
  fait le jour où le cas se présente en vrai. Tests de développement : Augmenta Fusion.
  Nouveau module **TrackSimu** (simulateur de blobs dans Chataigne, scènes 2D sur des
  plans X / Y / Z, à la main ou par mouvements humains / toucher), après le graphe nodal.
- **D17 (05/11)** **Surface** = un plan du monde réel (sol, mur), plus « scène » (mot d'Augmenta,
  gardé pour `/scene` et les scènes Fusion). Chaque surface a ses slots et sa sortie vidéo
  (flux Spout / NDI au nom de la surface). PSN : « JOUEUR 1 (SOL) », n° = 60000 + 100 x rang + slot (B4, 06/11).
  Zones en volume (brique 4) : bonus, le jour d'un tracking 3D (ZacTrack, lidar 3D).
