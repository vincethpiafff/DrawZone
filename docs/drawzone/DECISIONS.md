# Journal des décisions

Chaque décision avec sa raison, pour ne pas la remettre en question sans motif.
Ajouter les nouvelles décisions en bas, avec leur date.

## D1 — Partir de Chataigne (23/10/2026)

**Décision** : DrawZone est un fork de Chataigne, pas un logiciel écrit de zéro.

**Raisons** :
- Vincent connaît déjà l'interface de Chataigne.
- Protocoles déjà faits : OSC, OSCQuery, MIDI, DMX/Art-Net/sACN, UDP, TCP,
  HTTP, MQTT, WebSockets, PosiStageNet.
- Briques réutilisables : State Machine, Module Router, variables, Parrot
  (enregistrement/lecture de valeurs, utile pour rejouer du tracking),
  Dashboard, Detective.

**Conséquences** : C++/JUCE, licence GPL-3.0 obligatoire (convient : DrawZone
est open source). Contacter Ben Kuper une fois une base fonctionnelle.

## D2 — Un module « Spatial » plutôt qu'un logiciel à côté (23/10/2026)

**Décision** : les zones sont un nouveau type de module Chataigne et exposent
leurs valeurs comme paramètres Chataigne.

**Raison** : toutes les fonctions existantes (State Machine, Router, Timeline,
Parrot, Dashboard) fonctionnent alors sans rien réécrire.

## D3 — Logique façon game design (23/10/2026)

**Décision** : à terme, un éditeur nodal inspiré des Blueprints d'Unreal
(fils d'exécution et fils de données, événements entrée/sortie de zone,
Branch, Delay, DoOnce, Timelines, variables, machines à états, débogage en
direct), en complément de la State Machine de Chataigne.

**Raison** : concepts familiers aux créatifs venant du jeu vidéo, sans la
lourdeur d'un moteur généraliste.

## D4 — Windows uniquement pour commencer (23/10/2026)

**Décision** : compilation et tests sous Windows avec les Build Tools VS 2022.
Mac et Linux plus tard.

## D5 — 2D avant 3D (23/10/2026)

**Décision** : éditeur vue de dessus (v0.3) avant le viewport 3D (v0.5).

**Raison** : la plupart des zones sont au sol ; outil utile plus tôt ; la 3D,
plus risquée, arrive sur des fondations solides.

## D6 — Simulation avec Augmenta Fusion (23/10/2026)

**Décision** : tests avec Augmenta Fusion (possédé par Vincent). Pas de
simulateur public trouvé pour Naostage ; pour PSN, émetteur Python à écrire.

## D7 — Arborescence : JUCE à côté du dépôt (23/10/2026)

**Décision** : `D:\DRAWZONE\JUCE` et `D:\DRAWZONE\DrawZone`.

**Raison** : Ben interdit de placer JUCE dans le dépôt (voir son `AGENTS.md`).

## D8 — Projucer compilé en mode GPL (23/10/2026)

**Décision** : compiler le Projucer avec `JUCER_ENABLE_GPL_MODE=1`.

**Raison** : le Projucer standard refuse d'enregistrer un projet qui désactive
le splash screen JUCE sans licence commerciale. Le mode GPL est prévu par
JUCE pour les projets GPL, ce qu'est DrawZone. Aucune modification des
fichiers de JUCE ni du projet de Ben.

## D9 — Pas de conversion des fins de ligne (23/10/2026)

**Décision** : `core.autocrlf=false` dans le dépôt et ses sous-modules.

**Raison** : avec la conversion de Git for Windows, chaque enregistrement
dans le Projucer modifiait `BinaryData` et `JuceLibraryCode` (≈1000 lignes
parasites). Sans conversion, les fichiers sont identiques à ceux de Ben.

## D10 — Divergence minimale avec Chataigne (23/10/2026)

**Décision** : notre code dans nos propres fichiers ; dans le code de Ben,
seulement des points d'accroche minimaux commentés `// DrawZone:`. Le
`README.md` racine reste celui de Ben ; la présentation de DrawZone est dans
`.github/README.md` (GitHub l'affiche en priorité).

**Raison** : pouvoir récupérer les mises à jour de Ben sans conflits et lui
proposer notre travail plus tard.

## D11 — Fichier projet Windows versionné avec notre chemin JUCE (23/10/2026)

**Décision** : `Builds/VisualStudio2022/Chataigne_App.vcxproj`, régénéré par
notre Projucer, est commité (commit `3d851a53`, tag `v0.0.0`).

**Raison** : un clone du dépôt compile directement sur la machine de Vincent.
En cas de conflit avec upstream : reprendre la version de Ben puis
réenregistrer dans le Projucer.
