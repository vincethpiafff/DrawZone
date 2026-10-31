# TODO — DrawZone

Traçabilité de tout ce qui a été fait, étape par étape, avec les réussites
(✅), les échecs (❌) et leur correction. À lire par Claude Code au début de
chaque session, avec `CLAUDE.md`. Mettre à jour à la fin de chaque séance.

Légende : ✅ validé · ❌ échec (puis corrigé) · ⏭️ en cours · ⬜ à faire · 💡 idée

---

Phases 0 à 2 et versions v0.1 à v0.4 (terminées) : `docs/drawzone/TODO-archive.md`.

---

## ⏭️ En cours

- ⏭️ **v0.5 brique 4 : Tracking Monitor (couleur et liste des sources)**, puis brique 5 : test Augmenta + PSN, sur `feature/multi-source`.

## ⬜ À faire ensuite

- ⬜ **Étape de test importante avec la v0.6 (viewport 3D)** (demande de Vincent, 27/10/2026, placée avec le viewport 3D le 28/10) : vérifier le comportement de l'interface (Tracking Monitor, zones, slots) avec **plusieurs scènes**, puis avec **deux systèmes de tracking** en même temps (par exemple Augmenta Fusion en OSC + Naostage en PSN).

- ⬜ **Script Python pour SMODE : exposition automatique des valeurs OSC** (demande de Vincent, 28/10/2026) : à faire **une fois le graphe nodal et les variables de sortie terminés** (v0.7 / v0.8). Écrire un script Python pour SMODE qui crée automatiquement des paramètres SMODE à partir des valeurs OSC envoyées par DrawZone, sans les déclarer une par une à la main. Documentation : https://doc.smode.io/ (à étudier le moment venu : API Python de SMODE, réception OSC, création de paramètres).
  - ✅ étude (28/10, demande de Vincent, en avance) : la doc officielle ne publie pas l'API Python ; un device OSC de SMODE se relie aux paramètres à la main (Links, Learn), aucune méthode documentée pour lire ses messages depuis Python. Référence non officielle utilisée : github.com/gyomh/smode-oil-reference (ParameterBank dans `tools`, `Oil.createObject("Parameter(Real)")`, « configurer avant append », état par script, aucune exception ne doit sortir d'un script At Every Update) ;
  - ✅ 1re version : `Tools/drawzone/smode/drawzone_osc_bank.py` (Python Script, At Every Update) : écoute OSC elle-même sur un port dédié (Osc Port, 9000 par défaut), un paramètre par adresse dans la Parameter Bank de la scène / compo (créée si absente), mis à jour en temps réel ; f d → Real, i h → Integer, T F → Boolean, s → String, plusieurs valeurs → « /adresse [1] », « [2] » ; bundles ; Osc Clear ; Osc Status ; 20 créations max par image, 500 paramètres max ;
  - ✅ test hors SMODE avec le Python de SMODE (3.12.3) et un faux SMODE (`Oil`, `script`, bank simulés) + vrais paquets OSC en UDP : création, types, bundles, mise à jour, paramètre supprimé à la main puis recréé, paquet corrompu, Clear, Stop → OK ;
  - ⬜ **test de Vincent dans SMODE après la v0.7** (graphe nodal de base, avec ses sorties OSC ; décision de Vincent, 28/10) (types `Parameter(Real/Integer/String)` et `p.value.set` après l'append non confirmés par la référence) ;

- ⏭️ **v0.5 — entrée PSN, multi-sources, transformation par source** (démarrée le 28/10) :
  - ✅ étude : le module Spatial hérite du module OSC (un seul port, Augmenta) ; `TrackingData` est déjà indépendante du protocole (positions 0–1 + taille de scène venue d'Augmenta) ; Chataigne a un module PosiStageNet (bibliothèque `psn_lib` dans `External/posistagenet`, multicast 236.10.10.10:56565, décodage sur un thread) qui sait aussi **émettre** (Send Mode) → émetteur de test sans Python ; pas de TUIO dans Chataigne (à écrire : OSC sur UDP 3333) ;
  - ✅ proposition validée par Vincent : **Sources dans le module Spatial** (Augmenta, PSN, TUIO ; port et transformation Offset / Rotation / Scale / Flip par source ; ids uniques entre sources ; Scene Size Auto / Manual) ; **fusion** d'une personne vue par deux systèmes **reportée** (étape à part) ; tests PSN avec le module PSN de Chataigne (Send Mode) pendant le développement, puis **Naostage réel** pour valider ; briques : 1 socle multi-sources, 2 source PSN, 3 source TUIO, 4 Tracking Monitor (couleur et liste des sources), 5 test Augmenta + PSN ;
  - 💬 Vincent : simulateur Naostage sur leur site (« Enregistrement Demo (KAPTA Simulator) »), téléchargement impossible dans son navigateur ;
  - ✅ cause : le serveur envoie le fichier (1,8 Go) comme du texte brut (`Content-Type: text/plain`), le navigateur essaie de l'afficher → téléchargé par Claude avec curl : `D:\0-DRAWZONE\naostage\Demo_Festival.kramera` (hors dépôt) ; il se lit dans **Kratos** (logiciel Naostage, Windows : github.com/Naostage/kratos-releases, v1.10.14) : vue Simulator → Open file → lecture ; sortie PSN par un module PSN de Kratos (adresse multicast, port, X / Y / Z Remap ; Kratos a l'axe **Z vertical**, PSN standard a Y vertical → choix du plan du sol prévu dans la source PSN) ; doc : naostage.github.io/Kratos ;
  - ✅ brique 1, code : `TrackingData` multi-sources (chaque personne garde l'id de sa source s'il est libre, sinon un id libre à partir de 10000 ; silence et suppression par source ; personnes non mises à jour retirées par le minuteur) ; `SpatialSource.h/.cpp` : bloc **Sources** (manager avec fabrique, une source **Augmenta** créée d'office, sessions anciennes comprises), transformation par source (Offset, Rotation, Scale, Flip X / Y ; flip dans la zone Augmenta), Status ; l'entrée OSC du module alimente la première source Augmenta active (sous le verrou du manager) ; bloc **Scene** : Size Mode Auto (Augmenta) / Manual + Size ; Values > People : Source et Source Id ; fichiers déclarés dans le `.jucer` (Projucer : nos lignes seulement) ;
  - ❌ compilation : `BaseManager` crée ses éléments avec un constructeur sans argument → nom par défaut « Source » ;
  - ✅ compilation 0 erreur (exe du 28/10 13:02), Chataigne relancé par Claude ;
  - ✅ test Vincent : « ok ». **Brique 1 (socle multi-sources) validée** ;
  - 💬 Vincent : ajouter un « reset all parameters » ; l'Inspector du module Spatial devient chargé, settings difficiles à retrouver → propositions d'organisation (onglets ou autre) avec maquettes dans une page web (Claude artifact) ;
  - ✅ code : trigger **Reset All Parameters** dans chaque source (Offset, Rotation, Scale, Flip remis par défaut) ;
  - ✅ compilation 0 erreur (exe du 28/10 13:27), Chataigne relancé par Claude ;
  - ✅ page de propositions publiée (artifact privé, claude.ai/artifact/6A3CCx2aYwFt9BxFsEPJQM) : état actuel + A sections repliées / rangées / colorées avec résumé, B onglets en haut de l'Inspector, C barre d'icônes verticale (façon Blender), D réglages dans un tiroir du Tracking Monitor ; recommandation : A tout de suite, C ensuite ;
  - 💬 Vincent : choix « A puis C » ; avant de coder, voir plusieurs styles de sections sur l'Inspector complet ;
  - ✅ étude organicUI : chaque bloc peut avoir sa couleur (`contourColor` de l'éditeur de conteneur) et un éditeur à nous (`customGetEditorFunc`) pour dessiner l'en-tête et le résumé ; repli par défaut avec `editorIsCollapsed` ;
  - ✅ page des styles publiée (claude.ai/artifact/3Pt4vt4b29zCHGmURNgxi2) : A teinte (natif Chataigne), B liseré + pastille de résumé, C point de couleur + petites majuscules, D badge d'icône + puces d'état (icônes réutilisables pour C) ; recommandation : B ou D ;
  - 💬 Vincent : choix du style **D** ;
  - ✅ code : `ui/SpatialSectionEditor.h` : enveloppe générique des éditeurs organicUI (conteneur, enabling, manager, réseau) : badge d'icône à la couleur du bloc, puces d'état à droite (vertes = actif, mises à jour 4 fois par seconde), bordure colorée quand le bloc est ouvert ; `SpatialSectionManager<T>` pour Sources et Zones (le manager de Chataigne ignore `customGetEditorFunc`) ; ordre : Sources, Scene, Zones, Player Slots, Video Output, puis OSC Input / OSC Outputs / Pass-through ; tout replié à la création et au chargement (Chataigne ne sauvegarde que l'état replié) ; OSC Input garde son éditeur réseau (IP dans l'en-tête), badge seul ; OSC Outputs et Pass-through (managers de Ben) gardent leur look ;
  - ✅ compilation 0 erreur (exe du 28/10), Chataigne relancé par Claude ;
  - ✅ test Vincent : « très bien ». **Style D validé** (sections A en style D) ;
  - 💬 Vincent : Reset All Parameters validé (« ok ») ; nouvel ordre de l'Inspector, à garder pour les prochains blocs : OSC Input, OSC Outputs (+ Pass-through), futurs PSN / TUIO (entrées et sorties), Sources, Video Output, Zones, Player Slots ;
  - ✅ code : ordre appliqué dans `setupInspectorSections()` (tableau `order`) ; Scene, non citée, placée entre Video Output et Zones (à confirmer) ;
  - ✅ compilation 0 erreur (exe du 28/10 17:05), Chataigne relancé par Claude ;
  - 💬 Vincent : le module s'appelle désormais **Cockpit** (« aux manettes d'un vaisseau spatial ») ; choix : nom visible seul (le code garde Spatial), anciennes sessions lisibles ;
  - ✅ code : type de module « Cockpit » (menu Modules > DrawZone > Cockpit, message du Tracking Monitor) ; `ModuleFactory::create(type)` (code de Ben, marqué `// DrawZone:`) charge un ancien type « Spatial » comme Cockpit ;
  - ✅ compilation 0 erreur (exe du 28/10 17:08), Chataigne relancé par Claude ;
  - ✅ test Vincent : « tout est ok » (Cockpit dans le menu, TEST_ZONES.noisette rechargé avec ses zones, ordre des blocs) ;
  - ✅ commit sur `feature/multi-source` ;
  - ⏭️ brique 2 : source PSN (30/10) ; barre d'icônes verticale (C) plus tard :
    - ✅ code : `PSNSource` dans `SpatialSource.h/.cpp` (type « PSN » du bloc Sources) : thread et socket à elle, Multicast Address / Port, Up Axis (Y = standard PSN ; Z : y retourné pour une vue de dessus non inversée), Ignore Trackers At Origin (activé par défaut), position, hauteur, vitesse, orientation, âge calculé ; lecture dès qu'un paquet arrive (pas le `wait(10)` du module PSN de Ben, limité à 100 paquets / s) ; nouvel essai de connexion chaque seconde ; thread arrêté avant le retrait de la source ; pas de « leave » en PSN : retrait par le délai de 1 s ;
    - ✅ outils : `Tools/drawzone/sim/psn_sim.ps1` (remplace l'émetteur Python prévu : Python absent de la machine), `make_psn_session.ps1` (copie de session avec 2 sources PSN, hors dépôt), `check_psn.ps1` ;
    - ✅ compilation 0 erreur (seul C4101 de Ben, exe du 30/10 10:05) ; test PSN **18 / 18** (Y et Z, offset, vitesse, âge, tracker à l'origine ignoré, Silent, 2 ports, Augmenta + PSN avec le même id → 77 et ≥ 10000) ; non-régression sur `TEST_ZONES.noisette` **33 / 33** ; débit « 21 msg/s » normal : le simulateur envoie réellement 20,4 images / s ;
    - ⬜ non testé : Kratos / Naostage réel (sens des axes et signe de l'orientation à confirmer), désactiver ou supprimer une source en direct, machine à plusieurs cartes réseau (le multicast rejoint la carte par défaut, pas de choix d'interface), deux émetteurs sur le même groupe (mélangés) ; test de Vincent à faire ; pas de commit ;
    - ❌ 30/10 test Kratos : source « Silent » → bug **B2**. 💬 Vincent demande la taille de scène automatique depuis les données de Kratos. Constat : le PSN ne la transporte pas (capture de 30 s : 21,4 images / s, position + statut par tracker ; paquet d'info : « Kratos », LEAD, Target 2, 3, 4) ; X négatif (-1,59), donc origine différente du coin haut gauche de DrawZone ; show `.kratos` binaire propriétaire, port 2055 sans réponse HTTP. **Arrêt, décision avec Vincent** ;
    - 💬 30/10 Vincent (screenshot5) : les espaces scéniques sont dans Kratos > Area (TRACKING MAIN : Position -8, -2, 0 ; Scale 16, 12, 2) ; si la source vient de Kratos, l'utilisateur rentre ces informations. Position = coin (pas le centre) : déduit des trackers capturés, x = 1,96 serait hors d'une zone centrée en -8 ;
    - ⏭️ 30/10 code, **compilé mais pas testé** (édition de liens bloquée : Chataigne ouvert, session non enregistrée) : `PSNSource` Area Position / Area Size (Z up : scène x = x - pos.x, y = pos.y + size.y - y), nom du système dans Status, rappel si Kratos sans Up Axis Z ou sans Area ; Scene Size Mode « Auto (From Sources) » : 1re source activée qui connaît sa taille (relu dans le minuteur) ; B2 : groupe rejoint sur toutes les cartes (`IP_ADD_MEMBERSHIP`) ; `check_psn.ps1` et `psn_sim.ps1 -Via` mis à jour ;
    - ✅ 30/10 recompilation complète 0 erreur ; source PSN de la session : « Receiving, 21 msg/s, 4 people, from Kratos » (B2 corrigé). ❌ `check_sim.ps1` 27 / 33 sur la session : ce n'est pas une régression, les 4 personnes de Kratos prennent les 2 slots et entrent dans l'ellipse. → tests sur des copies de session sans Kratos (`make_psn_session.ps1`, ports 56575 / 56576, `-NoPsn`) : PSN **20 / 20**, Augmenta **33 / 33** ; lancer `check_sim.ps1` sur une copie `-NoPsn` si Kratos émet ; ⬜ Vincent : saisir Up Axis Z et Area, puis contrôle des positions Kratos ;
    - ✅ 30/10 B3 : compilation 0 erreur. ❌ 1er passage PSN 18 / 20 et Augmenta 29 / 33 : la session enregistrée a des paramètres PSN, le filtre de `make_psn_session.ps1` n'enlevait plus sa source, qui écoutait Kratos dans les copies → corrigé (erreur si la source reste). Puis PSN **20 / 20** (tracker à ≈ 0 ignoré), Augmenta **33 / 33**. Lecture Kratos en cours : tracker 4 bouge vraiment, 4 vraies personnes (brut 2,714 ; 2,593 → scène ≈ 10,75 ; 7,54 avec Area -8, -2 / 16, 12) ; ⬜ Vincent : sens de la vue (miroir), zones ; pas de commit ;
    - 💬 30/10 Vincent : sens de la vue OK ; activation / désactivation OK ; pas de scène prioritaire, il veut choisir sa référence ; zones réduites en passant de 16 x 8 à 16 x 12, à garder telles quelles. Constat : tailles en mètres identiques avant / après dans les fichiers (la vue change d'échelle pour montrer la scène entière), mais une scène plus petite pouvait les limiter (bornes = scène) ;
    - ✅ 30/10 code : `Use As Scene` par source (une seule à la fois ; sinon la seule source qui connaît sa scène ; plusieurs sans choix → pas de scène, message) ; bornes des zones fixes (1000 m), `SpatialZone::setSceneSize` supprimée ; tests ajoutés ;
    - ✅ 30/10 recompilation complète 0 erreur ; PSN source choisie **20 / 20**, PSN sans choix **20 / 20**, Augmenta **34 / 34** (dont zone intacte avec une scène de 2 x 1 m) ; ⬜ Vincent : case Use As Scene dans l'Inspector avec 2 sources ; pas de commit ;
    - ✅ 30/10 ordre des réglages des sources (`moveParametersToEnd`) : Use As Scene en premier ; scène, placement, réception, Status ; compilation 0 erreur. ❌ puis ✅ `make_psn_session.ps1` échouait car Vincent avait coché Use As Scene : il remplace maintenant toute la liste des sources. PSN **20 / 20**, Augmenta **34 / 34** ; ⬜ ordre à vérifier par Vincent dans l'Inspector ;
    - ✅ 30/10 Vincent : « ok commit », **entrée PSN validée** (sens de la vue, activation / désactivation, Use As Scene, ordre des réglages), commit sur `feature/multi-source`. Reste de la brique 2 : **PSN Output** (5 questions en attente) ; vue du Monitor à décider (toute la scène ou zoom à échelle fixe) ;
  - 💬 30/10 Vincent : toute la scène visible dans le Monitor pour le moment ; réglages d'interface et de workspace à faire **quand toutes les briques qui touchent au Tracking Monitor seront finies** ; repère de la sortie PSN validé (mètres de la scène, y = hauteur, z = y de la scène) ; autres propositions non contestées ;
  - ⏭️ **brique « PSN Output »** (demande de Vincent, 29/10/2026, à développer en même temps que l'entrée PSN) : l'utilisateur choisit s'il renvoie les trackers reçus (toutes sources) dans une **sortie PSN**, avec des **ids qui correspondent aux Player Slots** de DrawZone (slot 1 → tracker 1…) ; à préciser au moment de la brique : option on / off, personnes sans slot (non envoyées ou ids à part), slot tenu (« Held ») envoyé ou non, adresse multicast / port, bibliothèque `psn_lib` déjà dans `External/posistagenet` (elle sait émettre) ;
    - ⏭️ 30/10 code : `SpatialPSNOutput.h/.cpp` (bloc PSN Output, désactivé par défaut, 236.10.10.11:56565, envoi à 50 Hz sur une seule carte au choix (Network Interface, demande de Vincent : pas d'envoi sur toutes les cartes), slot N → tracker N, slot tenu envoyé à sa dernière position, sans vitesse, nom du slot dans le paquet d'info, avertissement si une source PSN écoute le même groupe) ; `.jucer` + Projucer ; recompilation complète 0 erreur ;
    - ❌ puis ✅ tests : les échecs venaient de Fusion en marche (personnes en plus) et de « Next Person Takes Slot » dans la session, pas du code. Copies de session isolées (OSC 12999, PSN 56575 / 56576, Hold Spot forcé), `run_checks.ps1` lance tout et rouvre la session : PSN **29 / 29** deux fois, Augmenta **36 / 36** ; ⬜ Vincent : réception dans un vrai logiciel PSN ; pas de commit ;
    - ✅ 30/10 Network Interface : recompilation complète 0 erreur. ❌ puis ✅ contrôle de la carte : un socket abonné sans carte entend aussi Loopback (constaté par un essai direct : envoi Loopback → Wi-Fi 0 / 20, sans carte 20 / 20), le test nomme donc une vraie carte. ❌ puis ✅ « source name » : la personne simulée finissait avant ce contrôle, durée portée à 9 s. PSN **29 / 29** deux fois, Augmenta **36 / 36**. 💬 Vincent : noms des ids = noms des slots (déjà le cas, paquet d'info envoyé chaque seconde, « JOUEUR 1 » testé, renommage en direct non testé) ; pas de commit ;
- ⏸️ 30/10/2026 fin de séance. Reprise : **vérifications de Vincent sur PSN Output** (carte réseau, trackers et noms dans un logiciel qui lit le PSN, renommage d'un slot, axes, slot tenu), puis commit sur son accord (rien n'est commité depuis `4916b7bb`) ; question ouverte : même choix de carte pour l'entrée PSN ? Ensuite : source TUIO (brique 3). Tests : `Tools/drawzone/sim/run_checks.ps1` ;
- ✅ 30/10 reprise (16:43) : `run_checks.ps1` relancé avec Fusion et Kratos en marche : PSN **29 / 29** deux fois, Augmenta **36 / 36**, session rouverte ; toujours en attente des vérifications de Vincent, pas de commit ;
- 💬 30/10 Vincent : 4 trackers dans Smode avec les ids de Kratos (screenshot 6) ; demande le choix des données envoyées par PSN Output et une proposition d'interface dans le Tracking Monitor. ❌ Constat : Smode affiche LEAD, Target 2, 3, 4, donc il écoute Kratos en direct (236.10.10.10) ; capture de la sortie DrawZone sur 236.10.10.11 : JOUEUR 1, JOUEUR 2. PSN Output reste à vérifier dans Smode ;
- ⏭️ 30/10 code, **pas compilé, pas testé** : paramètre Send de PSN Output (Player Slots, Tracking IDs, Source Data), `TrackedPerson` avec nom et valeurs brutes de la source, `check_psn_modes.ps1` ajouté à `run_checks.ps1`. ❌ `build_and_check.ps1 -Rebuild` arrêté avant la compilation : session Chataigne modifiée et non enregistrée (Chataigne non fermé). ⬜ Vincent : enregistrer la session, puis relancer. Proposition d'interface publiée (A pastille + menu, recommandée ; B sélecteur ; C barre Outputs) : claude.ai/artifact/JMafqX2vUYkafKvxLXmJkt ; ⬜ choix de Vincent ;
- ✅ 30/10 💬 Vincent : choix **A**. Code : pastille « PSN Output : <mode> (n) » dans l'en-tête du Monitor (case on / off, libellé → menu des modes + Show Settings). Recompilation complète 0 erreur. ❌ puis ✅ tests : `make_psn_session.ps1` gardait le bloc `psnOutput` de la session (activé par Vincent), puis utilisait `/sendMode` au lieu de `/send` ; erreurs du script de test, pas de l'option. PSN **29 / 29** deux fois, Tracking IDs **6 / 6**, Source Data **6 / 6**, Augmenta **36 / 36**. ⬜ Vincent : pastille à l'écran, 3 modes dans Smode sur 236.10.10.11 ; Kratos réel non testé en Source Data ; pas de commit (rien depuis `4916b7bb`) ;
- 💬 30/10 Vincent : « ça ne switch pas », Smode reçoit les valeurs brutes de Kratos quel que soit le mode. Cause : Smode écoute `236.10.10.10` (`Engine_15_9.configuration` : `multicastGroup`, log : « Creating device 236.10.10.10->127.0.0.1:56565 »), le groupe de Kratos ; la sortie DrawZone part sur 236.10.10.11 (capture : JOUEUR 1, JOUEUR 2). ⬜ Vincent : régler le device PSN de Smode sur 236.10.10.11. ⬜ changement de mode en direct jamais testé (tests : mode chargé depuis la session) ;
- 💬 30/10 Vincent : Smode réglé, le changement de mode marche en direct ; le repère doit être le même entre les modes **et entre une source Augmenta et une source Kratos**. Essai repère Kratos abandonné (❌ il change selon la source, jamais compilé). ⏭️ Code, **pas compilé, pas testé** : tout en mètres de la scène (x, y = hauteur, z = y scène) ; modes réduits à Player Slots et Source IDs ; valeurs brutes retirées de `TrackedPerson`. ✅ recompilation complète et `run_checks.ps1` OK (résultats ci-dessous) ; ⬜ Vincent : recaler Smode, alignement Augmenta / Kratos par Area et Offset ;
- ✅ 30/10 Vincent : « ok commit », **PSN Output validé** (Send : Player Slots / Source IDs, pastille du Monitor, repère de la scène), commit `a2a7a219` sur `feature/multi-source`. Question ouverte : choix de la carte pour l'entrée PSN ;
- 💬 30/10 Vincent : **TUIO abandonné** (décision D15), brique 3 retirée. Restent dans la v0.5 : brique 4, Tracking Monitor (couleur et liste des sources), et brique 5, test Augmenta + PSN ;
- ⏭️ 30/10 brique 4, code : rangée de pastilles des sources sous l'en-tête du Monitor (couleur, nom, réception, personnes ; clic → Inspector), couleur par source (`itemColor`, défaut donné par le manager), anneau à la couleur de la source autour de chaque personne (vue et liste People, pas la sortie vidéo ni le rail). Recompilation complète 0 erreur ; PSN **29 / 29** deux fois, Source IDs **6 / 6**, Augmenta **36 / 36** ; capture de la fenêtre : pastilles et anneaux affichés avec Kratos. ; le clic sur le libellé de PSN Output affiche ses réglages dans l'Inspector, plus de Show Settings. Vincent : « ok commit » ; clic sur une pastille, couleur et 2 sources actives non confirmés, à reprendre avec la brique 5 ;
- ✅ 30/10 Vincent : « ok tout bon », **brique 4 validée** (commit `c26abd93`). Suite : brique 5, test Augmenta + PSN ;
- ⏸️ 30/10/2026 fin de séance. Reprise : **brique 5, test Augmenta + PSN** (changement de source dans Smode, repère inchangé). Question ouverte : carte réseau de l'entrée PSN. Rien n'est poussé ; `TODO.md` à commiter avec le prochain commit ;
- ⏭️ 30/10 reprise, brique 5, 1er passage avec Fusion et Kratos réels, sur des copies de session avec Augmenta activé (`%TEMP%\drawzone_checks\both_slots.noisette`, `both_ids.noisette`), lecture seule : les 2 sources reçoivent en même temps (Augmenta 20 msg/s, scène 8 x 4,5 m ; PSN 21 msg/s, 4 personnes, from Kratos) ; scène 16 x 12 gardée (Use As Scene) ; sortie PSN cohérente, Player Slots : 2 trackers, JOUEUR 1, JOUEUR 2 ; Source IDs : 4 trackers, LEAD, Target 2, 3, 4 ; aucun avertissement dans les logs. ❌ **Test incomplet** : Fusion envoie 0 personne (aucune personne simulée), donc Augmenta et PSN jamais vus ensemble avec de vraies personnes en sortie. Session de Vincent inchangée (Augmenta désactivé), rouverte. ⬜ Vincent : lancer des personnes dans Fusion ; Smode n'est pas lancé, changement de source à vérifier dans Smode ;
- ⏭️ 30/10 brique 5, lecture sur la session de Vincent (Augmenta activé, personnes dans Fusion, Smode lancé), 3 lectures en 8 s : 6 personnes, 4 PSN (ids 1 à 4) + 2 Augmenta (0 et 10000 : id 1 déjà pris par Kratos, remplacement attendu) ; positions Augmenta dans sa scène 8 x 4,5 m, sans Offset ; scène 16 x 12 gardée ; aucun avertissement. Constats : les 2 Player Slots sont tenus par des personnes Kratos, donc aucune personne Augmenta en sortie dans ce mode ; positions Augmenta identiques aux 3 lectures (personnes immobiles ou flux figé, à trancher) ; 213 msg/s avec 2 personnes contre 20 à vide, non expliqué. ⬜ Vincent : bouger une personne dans Fusion, passer Send sur Source IDs, contrôler le repère dans Smode ;
- ✅ 30/10 flux Augmenta non figé : la personne 10000 a bougé (1,20 ; 1,63 → 1,94 ; 2,12 → 2,42 ; 0,85), la personne 0 reste immobile dans Fusion. ⬜ Send encore sur Player Slots ; débit de 213 msg/s toujours non expliqué ;
- ❌ 30/10 💬 Vincent : repères différents dans Smode (`screenshots/Screenshot_5.png`, Send sur Source IDs). Lecture : 6 trackers en sortie (0, 1, 2, 3, 4, 10000), noms Augmenta 0 / 1, LEAD, Target 2, 3, 4 ; Smode affiche les valeurs envoyées (Augmenta 0 : 3,78 ; 1,63 ; 1,84). Échelle et sens communs : projection de l'image cohérente pour les 2 sources (≈ 35 à 39 px / m). Cause : origines différentes, Augmenta part du coin de sa scène 8 x 4,5 m, Kratos est ramené au coin de son Area 16 x 12 m ; aucun alignement réglé (pas d'Offset). **Arrêt, décision avec Vincent** ; pas de code modifié ;
- ✅ 30/10 💬 Vincent : Offset retenu, 4 ; 3,75 saisi sur la source Augmenta. Lecture : Augmenta en (9,02 ; 5,60) et (5,69 ; 5,31), dans la zone centrée, valeurs identiques en sortie PSN. Pas de comparaison exacte avant / après : les personnes ont pu bouger entre les lectures ;
- 🐞 30/10 💬 Vincent : ids Augmenta 0 et 10000 anormaux. Constat dans le code (`TrackingData.cpp:89-94`) : l'id 1 de Fusion (« Augmenta 1 ») est déjà pris par Kratos, d'où 10000. Conforme au code mais **ids instables** : le premier arrivé garde son id, donc l'attribution dépend de l'ordre d'arrivée, et le compteur ne redescend jamais (une personne qui revient en conflit prend 10001, 10002…), ce qui casse les liens par id dans Smode. Correction proposée : plage d'ids par source (`Id Offset`). **Arrêt, décision avec Vincent** ; pas de code modifié ;
- ⏭️ 30/10 💬 Vincent : ok, Augmenta commence à 101. Code : `Id Offset` par source (Augmenta 101 par défaut, soit Fusion 0 → 101 ; PSN 0), id = id source + Id Offset, changement en direct pris en compte, 10000 gardé en secours avec avertissement dans Status ; `check_id_offset.ps1`. Recompilation complète 0 erreur ; PSN **30 / 30** deux fois, Source IDs **6 / 6**, Augmenta **36 / 36**. ❌ `check_id_offset.ps1` **3 / 4** : « Augmenta first » donne 1, 101, 102 au lieu de 1, 2, 101, 102 ; piste non prouvée : 2 `psn_sim` sur le même groupe, mélange déjà noté (ligne « deux émetteurs sur le même groupe »). **Arrêt, décision avec Vincent** ; changement d'Id Offset en direct non testé ; pas de commit ;
- 💬 30/10 Vincent, à faire : dans Smode, les trackers PSN doivent aussi porter le nom du système, comme « Augmenta 0 » pour Augmenta, donc « Kratos » pour les données PSN de Kratos (forme exacte à confirmer, par exemple « Kratos LEAD » ; aujourd'hui : « LEAD », « Target 2 ») ;
- ⏸️ 30/10/2026 fin de séance. Reprise : 1. nom « Kratos » des trackers PSN ; 2. refaire `check_id_offset.ps1` avec un émetteur par source PSN, plusieurs fois, et tester l'Id Offset en direct (Chataigne fermé environ 2 min, Vincent d'accord au préalable) ; 3. Vincent : repère dans Smode ; 4. commit sur son accord (`Id Offset` et outils non commités) ; débit Augmenta de 213 msg/s à expliquer ; question ouverte : carte réseau de l'entrée PSN ;
- ⏭️ 31/10 reprise. Code : nom « système tracker » des trackers PSN en sortie (« Kratos LEAD », forme choisie par Claude, à confirmer par Vincent). `check_id_offset.ps1` : un émetteur par source PSN (ports 56575, 56576) : **4 / 4 sur 3 passages**, le mélange de 2 émetteurs sur le même groupe était bien la cause du 3 / 4. Compilation 0 erreur (seul C4101 de Ben). ❌ 3 passages de `run_checks.ps1`, 1 sans erreur, 2 échecs isolés, non reproduits à l'identique : passage 1, PSN sans choix 29 / 30, « age grows » 6,56 s au lieu de moins de 3 s (session suivante et passages 2 et 3 : OK) ; passage 3, Augmenta 35 / 36, avertissements dans les logs (`Community Module definition file is not valid`, Spat Revolution, sur raw.githubusercontent.com ; message coupé à l'affichage). Causes non établies. **Arrêt, décision avec Vincent**. Non testé : Kratos réel (absent), Id Offset en direct. Pas de commit ;
- ⏭️ 31/10 💬 Vincent : « OK » (propositions de test et forme « Kratos LEAD »). Scripts, **pas encore lancés** : `check_psn.ps1` compare l'âge au temps réel depuis le départ de l'émetteur ; `check_psn.ps1` et `check_sim.ps1` ignorent seulement `Community Module definition file is not valid` et affichent les autres messages en entier. `run_checks.ps1` non relancé : il ferme Chataigne environ 3,5 min et Fusion a été lancé à 10:48, Vincent est peut-être en train de tester ; ✅ accord de Vincent (« go »), relancé ;
- ✅ 31/10 💬 Vincent : les trois vérifications sont OK dans Smode (noms « Kratos … », Id Offset en direct, repère Kratos / Augmenta) ;
- ✅ 31/10 `run_checks.ps1` avec les tests modifiés, 2 passages sans échec : PSN **30 / 30** (avec et sans choix), Source IDs **6 / 6**, Id Offset **4 / 4**, Augmenta **36 / 36**. Sur 2 passages seulement, le retard de lecture de « age grows » (cause supposée) et le 2e avertissement coupé du 31/10 ne sont pas réapparus ou pas identifiés. ✅ Vincent : « ok commit », commit sur `feature/multi-source`, non poussé. Restent ouverts : débit Augmenta de 213 msg/s, carte réseau de l'entrée PSN ;


## 🔧 Étape d'optimisation

Revue de tout le code DrawZone (doublons, améliorations, code mort,
robustesse, doc), faite **de temps en temps, à la demande de Vincent**.
Au démarrage d'une séance, Claude présente les tâches encore ouvertes et
Vincent choisit ce qu'on règle. Chaque tâche réglée passe à ✅ (avec la
date et le commit), une tâche écartée à ❌ avec la raison.

### Revue du 28/10/2026 (après la brique 1 de la v0.5, commit `f3c76b3b`)

Code relu : `Source/Module/modules/spatial/` (≈ 5 700 lignes),
`Tools/drawzone/`, `docs/drawzone/`. Rien n'a été modifié.
Recommandation : 1, 2, 4, 5 et 21 au début de la brique PSN (même code) ;
3 en une petite brique doc ; le reste dans une brique de nettoyage.

- 29/10/2026 : Vincent valide la recommandation (« ok »). Ordre : 3 (doc) ✅,
  puis 21, 4, 5, 1, 2 une par une (une brique = un test), puis le nettoyage.
- ⏸️ 29/10/2026 fin de séance (doc de la tâche 3 commitée : `9b33551f`). Reprise au
  prochain démarrage : tâche **21** (verrou sur `augmentaSceneSize`).
- 29/10/2026 reprise : tâche 21 écartée (❌, déjà protégée, commentaire
  corrigé). Suivante : tâche **4** (flip de la vitesse).
- 29/10/2026 tâche 4, code : `vectorToScene` gère le flip (changement de
  signe), `scaleAndRotate` (privée) commune à `toScene` et `vectorToScene`,
  2 lignes de flip retirées de `processPerson`. ❌ puis ✅ compilation :
  `C1083 juce_audio_basics.h` introuvable, car Vincent a renommé
  `D:\DRAWZONE` en `D:\0-DRAWZONE` (le projet pointe vers
  `D:\DRAWZONE\JUCE`). → Jonction Windows `D:\DRAWZONE` → `D:\0-DRAWZONE`
  (hors dépôt, choix de Vincent). Compilation 0 erreur (exe du 29/10 11:51,
  seuls C4101 / C4206 de Ben), Chataigne relancé par Claude. ⬜ test Vincent.
- 💬 29/10/2026 Vincent : « continue jusqu'au bout » → tâches 5, 1 et 2
  enchaînées, un seul test groupé à la fin.
- 29/10/2026 tâche 5, code : `removeSilentSources` supprimée (une personne
  d'une source muette n'est plus mise à jour, `removeStalePersons` la retire
  déjà au bout de la même seconde) ; `sourceTimeoutMs` ne sert plus qu'à
  afficher « Silent ».
- 29/10/2026 tâche 1, code : `TrackedPerson::position` en **mètres de la
  scène** ; plus de conversion 0-1 ↔ mètres dans les zones, les slots,
  People, le rendu, le Monitor (point, survol, attribution à la main,
  tableau) ; `updateFromPersons` et `PlayerSlots::update` sans taille de
  scène ; une personne Augmenta n'attend plus que le `/scene` de sa source.
- 29/10/2026 tâche 2, code : débit et dernier message **par source**
  (`TrackingData::SourceStats`, compteur `RateCounter` commun au module et
  aux sources) ; Status de chaque source : « Receiving, 50 msg/s, 1 people… »,
  « Silent » ou « No data yet » ; en-tête du Monitor : chaque source active
  et sa réception au lieu de « msg/s · Augmenta :port » ; texte « Waiting
  for scene size » écrit une seule fois (`SpatialModule::getNoSceneSizeText`),
  il cite aussi le mode Manual.
- ✅ 29/10/2026 compilation des tâches 5, 1 et 2 : 0 erreur (seul C4101 de
  Ben), Chataigne relancé par Claude. ⬜ test groupé de Vincent (4, 5, 1, 2).
- ❌ puis ✅ lecture MCP par Claude pendant le test : position en mètres
  (3.34, 2.54), slot « JOUEUR 1 », mais Status « Receiving, 116.486 msg/s » :
  dans JUCE, `String(float, 0)` ne veut pas dire 0 décimale (format par
  défaut). → `String(roundToInt(...))`. À recompiler à la prochaine
  fermeture de Chataigne.
- ✅ 29/10/2026 test Vincent : « ok tu peux commit ». **Tâches 4, 5, 1 et 2
  validées**, commit `3c5e5871` sur `feature/multi-source` (accord de Vincent). Reste
  de la revue : la brique de nettoyage (6 à 25, sauf 21) ; puis brique 2 de
  la v0.5 (source PSN) avec la brique « PSN Output ».
- 29/10/2026 brique de nettoyage (demande de Vincent : « enchaîne, teste
  chaque étape en simulation, arrête-toi à la moindre erreur ») :
  - ✅ outils de test dans `Tools/drawzone/sim/` : `augmenta_sim.ps1` (fausse
    personne Augmenta, id 77, en OSC sur 12000, à côté de Fusion),
    `check_sim.ps1` (scénarios zone rectangle / ellipse / dehors, vitesse,
    statut, départ par leave et par silence, logs ; lecture MCP seule),
    `build_and_check.ps1` (ferme Chataigne si la session n'a pas de
    modification, compile, relance, teste) ;
  - ❌ puis ✅ `check_sim.ps1` : MCP exige l'en-tête `MCP-Protocol-Version`
    et la notification `notifications/initialized` ; une erreur JSON-RPC
    arrive avec un statut HTTP d'erreur → gérée ;
  - ✅ référence sur la version commitée `3c5e5871` : 21 / 21 ;
  - ⏭️ tâche 6, code : zone de chaque personne calculée une fois dans
    `updatePeople` (`SpatialPerson::firstZoneIndex`), reprise par
    `TopViewModel` ; compilation 0 erreur ;
  - ❌ test 19 / 21 : positions relatives différentes. Cause : ce n'est pas le
    code (valeurs justes pour les zones rechargées) ; en fermant Chataigne,
    les zones déplacées pendant la séance de test de Vincent ont été perdues :
    le fichier `TEST_ZONES.noisette` (28/10 17:04) a d'autres zones, et
    Chataigne disait `modified: false`. Les autosaves de la séance ont été
    effacées à la fermeture. → **arrêt, décision avec Vincent**.
  - 💬 Vincent : garder les zones du fichier ; le bug va dans le nettoyage
    et se règle tout de suite ; « BUG = priorité la plus haute » ;
  - ✅ cause trouvée : bug **B1** (tableau 🐞) ; correction dans
    `ChataigneMcp.cpp` ; `check_sim.ps1` place désormais la personne au
    centre des zones lues dans la session (le test ne dépend plus de leur
    place) ; `chataigne_changes` refusé avec le profil Inspect (« Path is
    outside the approved folders ») ;
  - ✅ compilation 0 erreur (B1 + tâche 6 + arrondi du débit), simulation
    **21 / 21** (débit en entier : « 119 msg/s ») ;
  - ✅ test de Vincent pour B1 : zone déplacée, « * », session enregistrée (12:36). **B1 validé**, tâche 6 validée par la simulation ;
  - ✅ tâches 7 (`SpatialZone::toBox` / `fromBox`, `getBounds()` dans le rendu), 8 (`hitTest` → `zone->contains`), 9 (`setupAddedZone`), 10 (une seule liste de blocs) : compilation 0 erreur, simulation **32 / 32** (scénarios ajoutés : forme libre « Zone 1 », Person Count et Average Position des zones) ;
  - ✅ tâche 13 vérifiée : pas de bug (l'annulation d'un ajout enregistre l'état de la zone à ce moment, position comprise, donc rétablir la remet à sa place) ; doublon réglé : `TrackingMonitorPanel::addZone`, seul chemin de création (zone préparée avant l'ajout) ;
  - ❌ compilation de la tâche 13 : `C3859 Failed to create virtual memory for PCH` + `C1076 internal heap limit reached` sur 6 fichiers unity build (pas une erreur du code). Mémoire au moment du constat : 11,6 Go de RAM libres, 10 Go de mémoire virtuelle (commit) libres sur 49,8 Go ; Chataigne fermé par le script avant la compilation. → **arrêt, décision avec Vincent**.
    → 💬 Vincent : option 2 (fermer des logiciels : Capture 2026, Firefox) ; mémoire virtuelle libre 18,3 Go ; compilation OK, simulation 32 / 32 ;
  - ✅ tâche 15 (mode des slots choisi par sa valeur), 14 (énumérations `ZoneMenuItem`, `SlotMenuItem`, `PersonMenuItem`), 16 (contour des formes libres gardé en mémoire, refait quand la boîte ou un point change) : chacune compilée, simulation 32 / 32 ;
  - ⏭️ tâche 20, code : `Snapshot.frame`, `TrackingData::setScene` et `TrackingData::clear` supprimés, `#include "PlayerSlots.h"` retiré ; compilation 0 erreur ;
  - ❌ **Chataigne plante au démarrage** avec cette version : `0xc0000374` (corruption du tas, dans `ntdll.dll`), 1 s après le lancement, avant le chargement de la session (log arrêté après « Now sending to 127.0.0.1:9000 ») ; dump : `%LOCALAPPDATA%\CrashDumps\Chataigne.exe.7832.dmp`. Piste : `Snapshot` a changé de taille (champ `frame` retiré) et la compilation incrémentale n'aurait pas recompilé tous les fichiers qui l'utilisent. → **arrêt, décision avec Vincent**.
    → 💬 Vincent : recompilation complète. ✅ `/t:Rebuild` 0 erreur, Chataigne démarre, simulation 32 / 32 : **piste confirmée**, le code était bon. Leçon : après un champ ajouté ou retiré dans un en-tête, `build_and_check.ps1 -Rebuild` (option ajoutée). **Tâche 20 validée** ;
  - ❌ tâche 22 écartée (voir tableau) ;
  - ✅ tâche 19 : Status des sources refait à 4 Hz (`statusUpdateRateHz`), texte de résolution de la sortie vidéo à 4 Hz ; recompilation complète, simulation 32 / 32 ;
  - ⏭️ tâches 11 + 12, code : `ui/SpatialStyle.h` (nouveau) : `TrackingMonitorStyle` sorti de `TopViewRenderer.h`, palette unique (`activeColor` remplace `limegreen` dans le Monitor et le vert écrit en dur de `SpatialSectionEditor.h`, `trackBg`, `activeText`), aides `drawChip`, `drawChevron`, `drawTick`, `strokeDashed` utilisées partout ; `drawSlotModeIcon(bool holdSpot)` ; `.jucer` +2 lignes (CRLF, `perl`) ;
  - ❌ Projucer `--resave` : ≈ 1 550 lignes changées au lieu de quelques ajouts. Cause : depuis le renommage `D:\DRAWZONE` → `D:\0-DRAWZONE`, le chemin global du Projucer (`D:\DRAWZONE\JUCE`, via la jonction) est vu depuis `D:\0-DRAWZONE\DrawZone` : tous les chemins JUCE relatifs passent de `..\..\..\JUCE\…` à `..\..\..\..\DRAWZONE\JUCE\…` (VS 2022, CI, Win7CI ; Xcode : +2 lignes seulement, normal). Fonctionne grâce à la jonction, mais bruit énorme et dépendance à la jonction. Pas encore compilé. → **arrêt, décision avec Vincent**.
    → 💬 Vincent : option 1. ✅ chemin global du Projucer (`%APPDATA%\Projucer\Projucer.settings`) mis sur `D:\0-DRAWZONE\JUCE` ; `Builds/` remis à l'état commité puis `--resave` : 21 ajouts, 4 lignes changées (chemin JUCE absolu `D:\DRAWZONE\JUCE\modules` → `D:\0-DRAWZONE\JUCE\modules` dans le `.vcxproj` principal) ; chemins mis à jour dans `CLAUDE.md`, `BUILDING.md` et les Rappels ;
  - ✅ tâches 11 + 12 : recompilation complète 0 erreur, simulation 32 / 32 ; ⬜ coup d'œil de Vincent (puces, chevrons, coches, pointillés, vert de la pastille « Receiving ») ;
  - ✅ tâche 17 (1re partie) : 2 images réutilisées dans la sortie vidéo ; recompilation complète, simulation 32 / 32 ; Spout et NDI actifs dans la session pendant le test (« 800 x 450 px, 1 px = 1 cm ») → contrôle « video output renders » ajouté à `check_sim.ps1` : **33 / 33** ;
  - ✅ tâches 24 (`JOURNAL.md` figé avec un renvoi) et 25 (Phases 0 à 2 et v0.1 à v0.4 déplacées dans `docs/drawzone/TODO-archive.md`, contenu inchangé ; `TODO.md` : 721 → ≈ 245 lignes ; ligne cassée du 28/10 réparée : `D:\0-DRAWZONE\naostage\Demo_Festival.kramera`) ;
  - ⏸️ tâches 18 et 23 reportées (voir tableau). **Brique de nettoyage terminée** ; ✅ accord de Vincent pour le commit (« ok tu peux commit »), commits sur `feature/multi-source`.
  - ✅ session rouverte depuis `D:\0-DRAWZONE\ChataigneProject\TEST_ZONES.noisette` (vérifié par MCP) ; jonction `D:\DRAWZONE` supprimée par Vincent (`(Get-Item 'D:\DRAWZONE').Delete()` : `cmd /c rmdir` bloqué pour Claude, puis déformé par Git Bash avec `!`) ; JUCE, dépôt et session intacts ; plus aucun fichier de compilation ne pointe vers `D:\DRAWZONE`.
- ⏸️ 29/10/2026 fin de séance. Reprise : **v0.5 brique 2, source PSN** (avec la brique « PSN Output »), sur `feature/multi-source` ; les 2 lignes de fin de séance de `TODO.md` ne sont pas commitées.

#### 🐞 Bugs : priorité la plus haute (règle de Vincent, 29/10/2026)

| # | État | Où | Constat | Correction | Effort |
|---|---|---|---|---|---|
| B1 | ✅ 29/10 | `GlobalSettings` (organicUI, code de Ben) + `Source/MCP/ChataigneMcp.cpp` | Session jamais marquée modifiée tant qu'un client MCP est connecté : pas de « * » dans le titre, pas de question à la fermeture, modifications perdues (zones de la séance du 29/10). Cause : `GlobalSettings::onControllableFeedbackUpdate` remet le drapeau « modifié » à faux pour **tout** retour d'un de ses enfants, et le texte Activity des réglages MCP change à chaque requête (minuteur 2 Hz) | Bloc `// DrawZone:` dans `ChataigneMcp.cpp` (dépôt principal, pas de sous-module) : `GlobalSettings` n'écoute plus les retours du conteneur MCP ni des réglages de l'assistant. Effet de bord : changer un réglage MCP marque la session modifiée | Petit |
| B3 | ✅ 30/10 (simulation) | `PSNSource::processFrame` | Tracker inutilisé de Kratos compté comme une personne : il envoie 4,5e-18, -6,8e-19, 1,9e-17 et non 0 exact, donc Ignore Trackers At Origin ne le filtre pas (personne 4 à 8, 6 dans la scène) | Tolérance de 1 mm ; `psn_sim.ps1` envoie ces valeurs | Petit |
| B2 | ✅ 30/10 (simulation + réception Kratos) | `PSNSource::run` (`SpatialSource.cpp`) | Source PSN « Silent » avec Kratos sur la même machine : `DatagramSocket::joinMulticast` de JUCE rejoint le groupe sur la carte par défaut seulement (Wi-Fi : 1 paquet en 5 s), alors que Kratos émet sur Loopback (constaté le 30/10 : 56 paquets en 5 s). Non vu avec `psn_sim.ps1`, qui sort par la carte par défaut | À décider avec Vincent : rejoindre le groupe sur toutes les cartes (`IP_ADD_MEMBERSHIP` sur `getRawSocketHandle()`, JUCE non modifié) + paramètre Network Interface | Petit |

#### 🔴 Priorité haute : avant la source PSN

| # | État | Type | Où | Constat | Proposition | Effort |
|---|---|---|---|---|---|---|
| 1 | ✅ 29/10 | Conception | `TrackingData`, et partout | Positions rangées en 0-1 dans la scène puis remultipliées par la taille de scène à 9 endroits (zones, slots, People, rendu, Monitor…) ; une personne Augmenta est ignorée tant que la taille de scène est inconnue | Ranger les positions **en mètres** dans `TrackedPerson` (PSN envoie déjà des mètres) | Moyen |
| 2 | ✅ 29/10 | Spécifique Augmenta | Rendu, sortie vidéo, en-tête du Monitor | « Waiting for scene size (/scene) » écrit 2 fois ; « Augmenta :port » et un seul état Receiving / msg/s pour toutes les sources | État et débit **par source** (prévu en brique 4, à garder en tête dès PSN) | Moyen |
| 3 | ✅ 29/10 `9b33551f` | Doc | `DECISIONS`, `README`, `ROADMAP`, `CHANGELOG`, `CLAUDE.md`, commentaires | Renommage en **Cockpit** noté nulle part ; commentaire de `SpatialModule.h` dépassé (« Augmenta OSC for now… will expose zones ») | Fait : décision **D14** (D13 était déjà prise), CHANGELOG, CLAUDE.md, ROADMAP, en-tête de `SpatialModule.h` ; README sans mention du module, inchangé | Petit |
| 4 | ✅ 29/10 | Doublon | `AugmentaSource::processPerson` | Flip de la vitesse recodé à la main au lieu de passer par la transformation de la source (PSN en aura besoin) | Une seule fonction « vecteur → scène » qui gère le flip | Petit |

#### 🟠 Priorité moyenne : doublons et nettoyage

| # | État | Type | Où | Constat | Proposition | Effort |
|---|---|---|---|---|---|---|
| 5 | ✅ 29/10 | Doublon | Minuteur du module | `removeStalePersons` et `removeSilentSources` font presque la même chose (1 s chacun) ; le retour de la seconde n'est jamais lu | Garder une seule règle | Petit |
| 6 | ✅ 29/10 | Doublon | `SpatialModule::updatePeople` et `TopViewModel::update` | « Dans quelle zone est cette personne » cherché deux fois | Calculer une fois dans le module et réutiliser | Petit |
| 7 | ✅ 29/10 | Doublon | `SpatialZone`, `SpatialPerson`, `TopViewModel`, Monitor | Calcul « position dans la boîte ↔ mètres » réécrit 6 fois ; bornes recalculées au lieu de `getBounds()` | Deux petites fonctions communes | Petit |
| 8 | ✅ 29/10 | Doublon | `hitTest` du Monitor et `SpatialZone::contains` | Test « dans l'ellipse » recodé en coordonnées écran | `zone->contains(toMeters(pos))` | Petit |
| 9 | ✅ 29/10 | Doublon | `itemAdded` / `itemsAdded` (zones) | Même corps dans les deux | Une fonction commune | Petit |
| 10 | ✅ 29/10 | Doublon | `setupInspectorSections` | Blocs listés deux fois (liste « repliés » et liste « ordre ») | Une seule liste | Petit |
| 11 | ✅ 29/10 | Doublon (UI) | Monitor et `SpatialSectionEditor` | Dessins recopiés : puce (fond + bordure + survol) ×3, chevron ×2, coche ×2, pointillés ×5 ; deux palettes de couleurs, vert écrit en dur deux fois | Un petit fichier commun d'aides de dessin et une seule palette | Moyen |
| 12 | ✅ 29/10 | Rangement | `TopViewRenderer.h` | Le style du Monitor (`TrackingMonitorStyle`) est rangé dans l'en-tête du rendu | Le déplacer dans le fichier commun du point 11 | Petit |
| 13 | ✅ 29/10 | Doublon + bug possible | Création de zone dans le Monitor | 4 chemins de création ; `createZoneAt` place la zone **après** l'ajout (les autres avant) : après annuler / rétablir, la zone pourrait revenir au centre (**à vérifier**) | Un seul chemin de création | Petit |
| 14 | ✅ 29/10 | Lisibilité | Menus du Monitor | Numéros de menu « magiques » (1 à 9 dans le désordre) | Une énumération | Petit |
| 15 | ✅ 29/10 | Fragilité | Menu du mode des slots | Mode choisi par son texte (« Hold Spot To Reclaim ») recopié de `PlayerSlots` : casse si l'option est renommée | Choisir par valeur, pas par texte | Petit |

#### 🟢 Optimisation, code mort, robustesse, doc

| # | État | Type | Où | Constat | Proposition | Effort |
|---|---|---|---|---|---|---|
| 16 | ✅ 29/10 | Performance | `SpatialZone::contains` (forme libre) | Contour reconstruit à chaque test (par personne, par zone, 50 fois / s) | Garder le contour en mémoire, le refaire quand les points changent | Petit |
| 17 | ✅ 29/10 (1re partie) | Performance | `SpatialVideoOutput` | Nouvelle image allouée à chaque frame (≈ 8 Mo en 1080p, 30 fois / s) ; dé-prémultipliage et rendu sur le thread de l'interface | Fait : 2 images réutilisées (`getFrameImage`, une image encore lue par Spout ou NDI n'est pas reprise), effacées par `Image::clear`. Reste « rendre sur un thread à part » : à faire si le Monitor rame en 4K | Moyen |
| 18 | ⏸️ reportée | Performance | Monitor et sortie vidéo | Chacun recopie toutes les données (`TopViewModel`) de son côté | Reportée : les deux copies ne vont pas au même rythme (Monitor à l'écran, vidéo à son FPS) ; gain faible en 800 × 450. À mesurer avant de compliquer | Moyen |
| 19 | ✅ 29/10 | Performance (mineur) | Statut des sources, résolution vidéo | Textes reconstruits 50 (ou 30) fois / s | Rafraîchir à 4 Hz | Petit |
| 20 | ✅ 29/10 | Code mort | `TrackingData`, `PlayerSlots.cpp` | `Snapshot.frame` écrit mais jamais lu ; `TrackingData::clear()` jamais appelé ; `#include "PlayerSlots.h"` inutile en unity build | Supprimer | Petit |
| 21 | ❌ 29/10 | Robustesse | `AugmentaSource::augmentaSceneSize` | Écrit par le thread réseau, lu par le thread de l'interface, sans verrou | Écartée : constat faux. Les 3 accès (écriture dans `processMessage`, lectures dans `updateSceneSize` et `AugmentaSource::update`) se font déjà sous le verrou du manager des sources (`items.getLock()`, un `CriticalSection`). Seul le commentaire de `SpatialSource.h` (« network thread ») était trompeur : corrigé | Petit |
| 22 | ❌ 29/10 | Cosmétique | Ids remappés | Le compteur à partir de 10000 ne redescend jamais (ids très grands sur un long show) | Écartée : réutiliser un id qui vient de partir ferait croire à un slot « Hold » que sa personne revient (`PlayerSlots::update`, même id qui revient) ; des ids grands sont sans risque | Petit |
| 23 | ⏸️ reportée | Limite connue | Tracking Monitor | N'affiche que le premier module Cockpit | Reportée : c'est une fonction (choix du module dans le Monitor), à décider avec Vincent dans l'idée « Tracking Monitor plus user friendly » | Moyen |
| 24 | ✅ 29/10 | Doc en double | `docs/drawzone/JOURNAL.md` | Journal arrêté au 26/10, doublon de `TODO.md` | Le figer avec un renvoi vers `TODO.md`, ou le supprimer | Petit |
| 25 | ✅ 29/10 | Doc | `TODO.md` (≈ 570 lignes) | Devient long à relire | Archiver les versions terminées (v0.1 à v0.4) dans un fichier à part | Petit |

## 💡 Idées

Idées notées en route, datées, à trier plus tard (roadmap, décision ou abandon).
Format : `- 💡 JJ/MM/AAAA — idée (contexte, qui l'a proposée)`.

- 💡 24/10/2026 — **Slots de joueurs** (Vincent) : nombre fixe d'ids
  (« Joueur 1 », « Joueur 2 ») quel que soit le nombre de personnes
  détectées, avec une règle d'attribution (ex. la première personne détectée
  prend et garde le slot 1). → Intégrée à la roadmap en **v0.2**, après les
  métriques de zone.
- 💡 26/10/2026 — **Tracking Monitor plus « user friendly »** (Vincent) :
  la fenêtre fonctionne (en-tête, vue de dessus, tableau) mais son
  affichage est à retravailler plus tard. Pistes à trier : choix du module
  si plusieurs Spatial, vue et tableau redimensionnables, zoom, flèche de
  vitesse, défilement du tableau, colonnes au choix (hauteur, orientation…),
  personne survolée ↔ ligne du tableau. À faire « dans un temps plus
  lointain », pas en v0.1.
- 💡 26/10/2026 — **Vue 2D des zones envoyée en Spout / NDI** (Vincent) :
  sortie vidéo de la vue de dessus (zones + personnes), récupérable dans
  Smode, TouchDesigner, etc., pour voir les zones dans l'espace de
  projection. Déjà présent dans notre base : module `NDIOutModule`
  (`Source/Module/modules/video/`) et module `juce_sharedtexture`
  (Spout, `Modules/`). À étudier : rendu hors écran de la vue, résolution
  et calage sur la scène (1 px = x cm), fond transparent ou non.

## Rappels

- Une étape à la fois, avec la validation de Vincent. Pas de commit sans son accord.
- Compiler : `build_dev.bat` · Lancer : `start "" "Binaries\App\Chataigne.exe"`
- Projucer : `D:\0-DRAWZONE\JUCE\extras\Projucer\Builds\VisualStudio2022\x64\Release\App\Projucer.exe`
- Procédure complète et dépannage : `docs/drawzone/BUILDING.md`.
