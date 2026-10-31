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

## D12 — Le module Spatial hérite du module OSC de Chataigne (24/10/2026)

**Décision** : `SpatialModule` hérite de `OSCModule`, comme les modules
PowerPoint ou Millumin de Ben, et ne réécrit que `processMessageInternal()`.
Le module ne fait que **traduire** les messages Augmenta en une liste de
personnes (id, position en mètres) rangée dans une classe à nous,
indépendante d'OSC. Les zones ne lisent que cette liste.

**Raisons** :
- On récupère sans rien écrire : port d'entrée réglable, choix de la carte
  réseau, journal des messages, témoin d'activité, scripts, et les sorties
  OSC dont on aura besoin en v0.6.
- Code de Ben déjà testé, et notre code reste court.
- La liste de personnes indépendante d'OSC prépare la v0.4 : PSN et TUIO
  seront d'autres traducteurs vers la même liste.

**Conséquence** : deux modules ne peuvent pas écouter le même port. Pour
tester, désactiver ou supprimer le module OSC générique sur le port 12000.

**Alternative écartée** : un port ouvert directement par le module Spatial.
Plus de code, code de Ben en double, aucun gain en v0.1.

## D13 — Sortie vidéo NDI / Spout avant PSN / TUIO (28/10/2026)

**Décision** : la v0.4 devient la sortie vidéo de la vue 2D (vue de dessus
du Tracking Monitor) en NDI et Spout. Les versions suivantes sont décalées
d'un cran : PSN / TUIO en v0.5, viewport 3D en v0.6, graphe nodal en v0.7
et v0.8. Les numéros cités dans les décisions plus anciennes sont ceux de
l'époque.

**Raison** : choix de Vincent après la v0.2.0. La vue 2D est déjà riche
(zones, personnes, slots, emplacements réservés) et sert tout de suite en
spectacle, comme retour visuel ou source vidéo.

## D14 — Le module s'appelle « Cockpit », le code garde « Spatial » (28/10/2026)

**Décision** : le nom visible du module devient **Cockpit** (menu
Modules > DrawZone > Cockpit, type de module, messages). Le code garde le
nom de travail **Spatial** (`SpatialModule`, dossier
`Source/Module/modules/spatial/`, commits `feat(spatial)`).

**Raisons** :
- Choix de Vincent : « aux manettes d'un vaisseau spatial ».
- Renommer tout le code coûterait cher pour rien : seul le nom affiché
  compte pour l'utilisateur.

**Conséquence** : `ModuleFactory::create(type)` (code de Ben, marqué
`// DrawZone:`) charge un ancien type « Spatial » comme Cockpit, donc les
anciennes sessions s'ouvrent toujours. Dans la doc, « module Spatial » et
« module Cockpit » désignent le même module.

## D15 — Abandon de l'entrée TUIO (30/10/2026)

**Décision** : DrawZone n'aura pas d'entrée TUIO ; la brique 3 de la v0.5
est retirée. Les entrées de tracking restent Augmenta (OSC) et PSN.

**Raison** : choix de Vincent.

**Conséquence** : rien à retirer dans le code, TUIO n'avait pas commencé.
Les mentions de TUIO dans les décisions et le journal passés restent telles
quelles, comme trace de ce qui était prévu.
