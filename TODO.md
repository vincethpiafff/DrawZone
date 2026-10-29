# TODO — DrawZone

Traçabilité de tout ce qui a été fait, étape par étape, avec les réussites
(✅), les échecs (❌) et leur correction. À lire par Claude Code au début de
chaque session, avec `CLAUDE.md`. Mettre à jour à la fin de chaque séance.

Légende : ✅ validé · ❌ échec (puis corrigé) · ⏭️ en cours · ⬜ à faire · 💡 idée

---

Phases 0 à 2 et versions v0.1 à v0.4 (terminées) : `docs/drawzone/TODO-archive.md`.

---

## ⏭️ En cours

- ⏭️ Brique de nettoyage de la revue du 28/10 (section « 🔧 Étape d'optimisation »), puis **v0.5 brique 2 : source PSN** (avec la brique « PSN Output »), sur `feature/multi-source`.

## ⬜ À faire ensuite

- ⬜ **Étape de test importante avec la v0.6 (viewport 3D)** (demande de Vincent, 27/10/2026, placée avec le viewport 3D le 28/10) : vérifier le comportement de l'interface (Tracking Monitor, zones, slots) avec **plusieurs scènes**, puis avec **deux systèmes de tracking** en même temps (par exemple Augmenta Fusion en OSC + Naostage en PSN).

- ⬜ **Script Python pour SMODE : exposition automatique des valeurs OSC** (demande de Vincent, 28/10/2026) : à faire **une fois le graphe nodal et les variables de sortie terminés** (v0.7 / v0.8). Écrire un script Python pour SMODE qui crée automatiquement des paramètres SMODE à partir des valeurs OSC envoyées par DrawZone, sans les déclarer une par une à la main. Documentation : https://doc.smode.io/ (à étudier le moment venu : API Python de SMODE, réception OSC, création de paramètres).
  - ✅ étude (28/10, demande de Vincent, en avance) : la doc officielle ne publie pas l'API Python ; un device OSC de SMODE se relie aux paramètres à la main (Links, Learn), aucune méthode documentée pour lire ses messages depuis Python. Référence non officielle utilisée : github.com/gyomh/smode-oil-reference (ParameterBank dans `tools`, `Oil.createObject("Parameter(Real)")`, « configurer avant append », état par script, aucune exception ne doit sortir d'un script At Every Update) ;
  - ✅ 1re version : `Tools/drawzone/smode/drawzone_osc_bank.py` (Python Script, At Every Update) : écoute OSC elle-même sur un port dédié (Osc Port, 9000 par défaut), un paramètre par adresse dans la Parameter Bank de la scène / compo (créée si absente), mis à jour en temps réel ; f d → Real, i h → Integer, T F → Boolean, s → String, plusieurs valeurs → « /adresse [1] », « [2] » ; bundles ; Osc Clear ; Osc Status ; 20 créations max par image, 500 paramètres max ;
  - ✅ test hors SMODE avec le Python de SMODE (3.12.3) et un faux SMODE (`Oil`, `script`, bank simulés) + vrais paquets OSC en UDP : création, types, bundles, mise à jour, paramètre supprimé à la main puis recréé, paquet corrompu, Clear, Stop → OK ;
  - ⬜ **test de Vincent dans SMODE après la v0.7** (graphe nodal de base, avec ses sorties OSC ; décision de Vincent, 28/10) (types `Parameter(Real/Integer/String)` et `p.value.set` après l'append non confirmés par la référence) ;

- ⏭️ **v0.5 — entrées PSN et TUIO, multi-sources, transformation par source** (démarrée le 28/10) :
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
  - ⏭️ brique 2 : source PSN ; barre d'icônes verticale (C) plus tard ;
  - ⬜ **brique « PSN Output »** (demande de Vincent, 29/10/2026, à développer en même temps que l'entrée PSN) : l'utilisateur choisit s'il renvoie les trackers reçus (toutes sources) dans une **sortie PSN**, avec des **ids qui correspondent aux Player Slots** de DrawZone (slot 1 → tracker 1…) ; à préciser au moment de la brique : option on / off, personnes sans slot (non envoyées ou ids à part), slot tenu (« Held ») envoyé ou non, adresse multicast / port, bibliothèque `psn_lib` déjà dans `External/posistagenet` (elle sait émettre) ;


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

#### 🐞 Bugs : priorité la plus haute (règle de Vincent, 29/10/2026)

| # | État | Où | Constat | Correction | Effort |
|---|---|---|---|---|---|
| B1 | ✅ 29/10 | `GlobalSettings` (organicUI, code de Ben) + `Source/MCP/ChataigneMcp.cpp` | Session jamais marquée modifiée tant qu'un client MCP est connecté : pas de « * » dans le titre, pas de question à la fermeture, modifications perdues (zones de la séance du 29/10). Cause : `GlobalSettings::onControllableFeedbackUpdate` remet le drapeau « modifié » à faux pour **tout** retour d'un de ses enfants, et le texte Activity des réglages MCP change à chaque requête (minuteur 2 Hz) | Bloc `// DrawZone:` dans `ChataigneMcp.cpp` (dépôt principal, pas de sous-module) : `GlobalSettings` n'écoute plus les retours du conteneur MCP ni des réglages de l'assistant. Effet de bord : changer un réglage MCP marque la session modifiée | Petit |

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
