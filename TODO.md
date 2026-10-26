# TODO — DrawZone

Traçabilité de tout ce qui a été fait, étape par étape, avec les réussites
(✅), les échecs (❌) et leur correction. À lire par Claude Code au début de
chaque session, avec `CLAUDE.md`. Mettre à jour à la fin de chaque séance.

Légende : ✅ validé · ❌ échec (puis corrigé) · ⏭️ en cours · ⬜ à faire · 💡 idée

---

## Phase 0 — Conception (23/10/2026, claude.ai)

- ✅ Idée : outil pour dessiner des zones d'interaction alimentées par du
  tracking (Augmenta, Naostage, ZacTrack ; OSC, PSN, TUIO), avec logique
  conditionnelle et sorties OSC (booléens, %, durées).
- ✅ Évolution : logique « façon game design » inspirée des Blueprints Unreal.
- ✅ Choix de partir d'un fork de Chataigne (Ben Kuper, C++/JUCE, GPL-3.0).
- ✅ Choix : module « Spatial » dont les zones exposent des paramètres Chataigne.
- ✅ Choix : Windows d'abord, 2D avant 3D, tests avec Augmenta Fusion.
- ✅ Nom provisoire : DrawZone. Roadmap v0.0 → v2.x définie
  (`docs/drawzone/ROADMAP.md`).

## Phase 1 — Installation et v0.0 (23/10/2026)

| # | Étape | Résultat | Détail / correction |
|---|---|---|---|
| 1.1 | Télécharger Build Tools VS 2022 via `aka.ms` | ❌ | Firefox : « Server Not Found » sur aka.ms. → Installation via la page « older downloads » / winget. |
| 1.2 | Installer Build Tools VS 2022 | ✅ | Charge « Développement Desktop en C++ », MSVC v143, SDK Windows 11 (10.0.26100). |
| 1.3 | Git for Windows + compte GitHub | ✅ | Déjà présents. |
| 1.4 | Fork GitHub de Chataigne renommé DrawZone | ✅ | https://github.com/vincethpiafff/DrawZone |
| 1.5 | Cloner JUCE de Ben (`develop-local`) dans `D:\DRAWZONE\JUCE` | ✅ | JUCE 7.0.12. Hors du dépôt (règle de Ben). |
| 1.6 | Cloner DrawZone avec sous-modules | ✅ | 6 sous-modules (organicui, timeline, simpleweb, sharedtexture, serial, dmx). |
| 1.7 | Remotes et branche `develop` | ✅ | `upstream` = Ben, push bloqué (`NE_PAS_PUSHER`). `develop` poussée. |
| 1.8 | Pack de dépendances téléchargé | ✅ | `dependencies_zips\Chataigne-win-x64-release-dependencies.zip`, 28 Mo. |
| 1.9 | Première compilation `build_dev.bat` | ❌ | `C1083: Cannot open source file 'JucePrecompiledHeader_Release.cpp'`. Cause : projet VS généré sur la machine de Ben (`D:\Projects\Dev\JUCE`), fichiers PCH non versionnés. → Il faut le Projucer. |
| 1.10 | Compiler le Projucer | ❌ puis ✅ | `'msbuild' is not recognized` (pas dans le PATH). → Chemin complet de MSBuild. Build OK. |
| 1.11 | Enregistrer `Chataigne.jucer` dans le Projucer | ❌ | « Incompatible License and Splash Screen Setting — Save and export is disabled ». |
| 1.12 | Projucer recompilé en mode GPL | ✅ | `set CL=/DJUCER_ENABLE_GPL_MODE#1` + Rebuild. Logo GPL affiché. |
| 1.13 | Chemins globaux du Projucer | ✅ | Path to JUCE = `D:\DRAWZONE\JUCE`, Modules = `D:\DRAWZONE\JUCE\modules`. |
| 1.14 | Enregistrement du projet | ❌ | ~1000 lignes modifiées dans `BinaryData.cpp/.h` + avertissements LF/CRLF sur tout `JuceLibraryCode`. Cause : conversion automatique LF→CRLF de Git for Windows. |
| 1.15 | Reclone propre avec `core.autocrlf=false` (dépôt + sous-modules) | ✅ | Ancien clone renommé `DrawZone_old`. Après Ctrl+S : un seul fichier modifié, `Builds/VisualStudio2022/Chataigne_App.vcxproj`. |
| 1.16 | Deuxième compilation | ❌ | `C1083: Cannot open include file: 'WebView2.h'`. Cause : paquet NuGet non restauré (pas d'IDE). |
| 1.17 | Copie du paquet WebView2 depuis `Builds\VisualStudio2022_CI\packages` | ✅ | Même version 1.0.902.49. |
| 1.18 | Troisième compilation | ✅ | Build succeeded, 0 erreur, 2 avertissements du code de Ben (C4101, C4206). ~3 min. |
| 1.19 | Premier lancement | ❌ | DLL manquantes : JoyShockLibrary, Kinect20, Servus, SDL2. Cause : pack non extrait. |
| 1.20 | Extraction du pack dans `Binaries\App` | ✅ | Chataigne démarre. Console : `DNSServiceDiscovery error -65563` = Bonjour absent, sans conséquence. |
| 1.21 | Test Augmenta Fusion → module OSC port 12000 | ✅ | Reçus : `/scene`, `/fusion`, `/object/update`, `/object/leave`, `/object/leave/extra`. |
| 1.22 | Commit du `.vcxproj` régénéré + tag | ✅ | Commit `3d851a53`, tag `v0.0.0`, poussés. |

**v0.0.0 validée le 23/10/2026.**

## Phase 2 — Documentation et transfert vers VS Code (23–24/10/2026)

| # | Étape | Résultat | Détail / correction |
|---|---|---|---|
| 2.1 | Rédaction des fichiers de transfert | ✅ | `CLAUDE.md`, `.github/README.md`, `docs/drawzone/` (BUILDING, ROADMAP, DECISIONS, CONTRIBUTING, CHANGELOG, JOURNAL, notes Augmenta). |
| 2.2 | Script `Tools/drawzone/setup_windows.bat` | ✅ | Automatise les étapes 1.8, 1.17 et 1.20. Sans effet s'il est relancé. |
| 2.3 | Document de présentation pour un développeur | ✅ | Doc claude.ai + PDF « DrawZone — présentation et architecture ». |
| 2.4 | Extraction du zip dans le dépôt | ❌ puis ✅ | Zip cherché dans `D:\DRAWZONE` alors qu'il était dans Téléchargements. → `tar -xf "%USERPROFILE%\Downloads\drawzone-docs.zip" -C .` |
| 2.5 | Test de `setup_windows.bat` sous Windows | ✅ | « already installed / downloaded / extracted ». |
| 2.6 | Relecture de `CLAUDE.md` par Vincent | ✅ | Validé sans modification. |
| 2.7 | Commit de la documentation | ✅ | Commit `33ea7c48` sur `develop`, poussé. |
| 2.8 | Ajout de ce `TODO.md` | ✅ | Committé sur `develop` avec la mise à jour de `CLAUDE.md` et `JOURNAL.md`. |
| 2.9 | Ouverture dans VS Code + première session Claude Code (24/10) | ❌ puis ✅ | Première lecture : `TODO.md` introuvable, car le fichier a été ajouté au dépôt pendant la séance. → Relu ensuite, résumé du projet validé. |
| 2.10 | Rôle de `TODO.md` défini | ✅ | Document de traçabilité du projet : réussites, échecs et corrections, idées. Mentionné dans `CLAUDE.md` (section 2). Section « Idées » ajoutée. |
| 2.11 | Ménage : suppression de `D:\DRAWZONE\DrawZone_old` | ✅ | Constaté le 24/10 : le dossier n'existe plus. |
| 2.12 | Dossier de captures d'écran pour Claude Code | ❌ puis ✅ | Créé d'abord dans le dépôt (`docs/drawzone/screenshots/`) : malentendu. → Supprimé et recréé hors du dépôt : `D:\DRAWZONE\screenshots\`, donc rien à ignorer côté Git. |
| 2.13 | Vocabulaire | ✅ | « le repo » = `D:\DRAWZONE` ; « le dossier DrawZone » = `D:\DRAWZONE\DrawZone` (dépôt Git). |
| 2.14 | Vérifier que la CI de Ben est désactivée sur le fork | ❌ puis ✅ | Capture du 24/10 : la CI est **active**. Le push `419bf011` a déclenché `build.yml` (#1, « Queued »). Ce workflow se lance à chaque push (`on: [push]`) sur 4 machines GitHub (Windows, macOS, 2× Linux) et utilise les secrets de Ben (absents chez nous). → Actions désactivées par Vincent (Settings → Actions → General). Annulation du run #1 : erreur GitHub « Cannot cancel a check suite that is completed » alors que le run est affiché « In progress ». Sans conséquence : le run échouera seul (secrets absents), rien n'est envoyé chez Ben. |
| 2.15 | Audit « ne pas déranger Ben » (local) | ✅ | `upstream` en push `NE_PAS_PUSHER` ✅ ; `core.autocrlf=false` partout ✅ ; seul workflow : `build.yml` ✅. Point faible : les 6 sous-modules et `D:\DRAWZONE\JUCE` ont encore l'URL de push de Ben (un push serait refusé, mais autant le bloquer). |
| 2.16 | Push bloqué sur les 6 sous-modules et sur JUCE | ✅ | `git remote set-url --push origin NE_PAS_PUSHER`. Test : un push à blanc échoue bien (« 'NE_PAS_PUSHER' does not appear to be a git repository »). La récupération (fetch) depuis Ben fonctionne toujours. Réglage local, à refaire après un nouveau clone. |
| 2.17 | Règles « ne pas déranger Ben » dans `CLAUDE.md` (section 5) | ✅ | Pas de PR vers Ben, pas de mention `@benkuper` ni de lien vers ses tickets, Actions désactivées, « Sync fork » sans risque. |

## ⏭️ En cours

- *(rien en cours : Phase 2 terminée, prochaine étape la v0.1)*

## ⬜ À faire ensuite

- ⬜ **v0.1 — squelette du module Spatial** :
  - ✅ lire la spécification Augmenta (24/10). ❌ puis ✅ : la page « Data » du
    wiki renvoie vers docs.augmenta.tech, où la page du protocole est
    introuvable. → Ordre des arguments repris du code du plugin officiel
    Unreal, recoupé avec nos observations. Résultat dans
    `docs/drawzone/notes/augmenta-protocol.md` ;
  - ✅ étudier les modules OSC et PosiStageNet de Chataigne (24/10). Les modules
    « Software » (PowerPoint, Millumin…) héritent de `OSCModule` et ne
    réécrivent que `processMessageInternal()`. Enregistrement = 3 lignes
    (`ModuleIncludes.h`, `ModuleIncludes2.cpp`, `ModuleFactory.cpp`) ;
  - ✅ décider : récepteur OSC de Chataigne réutilisé → **D12** validée par
    Vincent le 24/10 (`SpatialModule` hérite de `OSCModule`, liste de
    personnes indépendante d'OSC) ;
  - ✅ créer la branche `feature/spatial-module` (24/10, depuis `develop`) ;
  - ⏭️ module vide enregistré dans Chataigne, compilé et visible dans le menu :
    - ✅ `Source/Module/modules/spatial/SpatialModule.h/.cpp` (hérite de
      `OSCModule`, port d'entrée 12000, sortie 9000) ;
    - ✅ 3 points d'accroche `// DrawZone:` : `ModuleIncludes.h`,
      `ModuleIncludes3.cpp`, `ModuleFactory.cpp` (catégorie « DrawZone ») ;
    - ❌ puis ✅ ajout au `.jucer` : un `sed` de Git Bash a converti tout le
      fichier en LF (≈4000 lignes modifiées). → Fichier restauré
      (`git checkout`), insertion refaite avec l'outil d'édition : 6 lignes.
      Leçon : ne jamais modifier le `.jucer` (CRLF) avec `sed` ;
    - ❌ puis ✅ Projucer `--resave` : lancé directement, il rend la main avant
      d'avoir fini (rien régénéré). → `Start-Process -Wait` : 8 exports
      régénérés, uniquement des ajouts de nos 2 fichiers (77 lignes) ;
    - ✅ compilation `build_dev.bat` : 0 erreur, 1 avertissement du code de Ben
      (C4101), 2 min 26 ;
    - ⬜ test par Vincent : module visible dans le menu, réception OSC ;
  - ⏭️ **workflow de test « Claude lance Chataigne et lit les logs »** (24/10) :
    - ✅ étape 1 — couper tout envoi vers les serveurs de Ben. Trouvé dans le
      code : vérification des mises à jour, envoi des rapports de plantage
      (« Auto send crash log » activé par défaut), statistiques d'usage Matomo
      (au démarrage, à l'arrêt, au plantage, toutes les 5 min), conversion en
      ligne des anciens fichiers `.noisette` (le fichier entier est envoyé).
      → Bloc `// DrawZone:` de 8 lignes dans `Source/Main.cpp`, après les
      lignes de Ben (sans les modifier) : adresses vidées. Les simples
      lectures (page d'accueil, liste des modules communautaires, aide) sont
      gardées. ❌ puis ✅ : `Main.cpp` mélange CRLF et LF ; l'outil d'édition
      avait tout converti en CRLF → fichier restauré, insertion à l'octet près
      avec `perl`. Compilation : 0 erreur, 0 avertissement ;
    - ✅ étape 2 — Claude lance Chataigne en arrière-plan (console captée,
      notification si arrêt ou plantage). Vincent a coché Preferences →
      Save / Load → « Save logs » → fichier créé :
      `%APPDATA%\Chataigne\log_<date>_<heure>.txt` (un par lancement ; les
      messages d'avant la case cochée n'y sont pas) ;
    - ⏭️ étape 2 bis — **brancher Claude Code sur le serveur MCP de Chataigne**
      (travail de Ben déjà compilé dans notre build : `Source/MCP/`,
      `Modules/juce_organicui/mcp/`, doc `Tools/mcp/CLIENTS.md`). Règle :
      Claude en **lecture seule** (profil Inspect), Vincent fait toutes les
      actions en utilisateur.
      - ✅ Preferences → MCP : client « Claude Code », Access = Inspect,
        Advanced execution décoché, Enabled coché. Serveur à l'écoute sur
        `127.0.0.1:42002` (vérifié) ;
      - ✅ test `curl` sans jeton : HTTP 401 « Authentication required »
        (le serveur répond et exige bien un jeton) ;
      - ❌ jeton dans la variable utilisateur `CHATAIGNE_MCP_TOKEN` :
        1er essai, variable absente du registre (lieu d'exécution de `setx`
        inconnu) ; 2e essai, la variable contenait le texte d'exemple de la
        consigne `<colle le jeton ici>` (consigne ambiguë de Claude) ;
        3e essai, longueur 0. Un jeton valide fait **64 caractères**.
      - 26/10 : on abandonne `setx`, nouvelle méthode : **Claude lit le
        presse-papier** (`Get-Clipboard`), donc plus d'erreur de
        copier-coller. Séance interrompue (redémarrage du PC) avant la copie ;
      - ✅ 26/10 : jeton lu dans le presse-papier (64 caractères, jamais
        affiché), enregistré dans la variable utilisateur
        `CHATAIGNE_MCP_TOKEN` et dans `D:\DRAWZONE\.secrets\chataigne_mcp_token.txt`
        (hors dépôt) ;
      - ❌ puis ✅ test HTTP : `initialize` → 200 (Chataigne 1.10.5b6), mais
        `tools/list` → 404. Cause : chaque requête doit renvoyer l'en-tête
        `Mcp-Session-Id` donné par `initialize` (règle du protocole MCP).
        → Avec l'en-tête : 20 outils listés, `chataigne_diagnostics` renvoie
        les logs et `"readOnly": true` (profil Inspect confirmé) ;
      - ✅ `D:\DRAWZONE\.mcp.json` créé (modèle de `Tools/mcp/CLIENTS.md`,
        le jeton n'y est pas écrit : `${CHATAIGNE_MCP_TOKEN}`) ;
      - ⏭️ Vincent relance Claude Code depuis `D:\DRAWZONE`
        (`claude --continue`), approuve le serveur « chataigne », vérifie
        avec `/mcp`. Chataigne doit être lancé avant ;
      - ❌ 26/10 : Claude Code relancé depuis `D:\DRAWZONE`, serveur
        « chataigne » reconnu, mais connexion refusée (`ECONNREFUSED`).
        Cause : Chataigne n'était pas lancé (aucun processus, port 42002
        fermé). → Lancer Chataigne, puis `/mcp` → reconnecter « chataigne » ;
      - ✅ 26/10 : Claude lance Chataigne en arrière-plan ; port 42002 ouvert
        au bout de 25 s (le serveur répond) ;
      - ✅ 26/10 : Vincent `/mcp` → Reconnect. Outils `mcp__chataigne__*`
        disponibles dans Claude Code ; `chataigne_project` lit la session
        (vide, non enregistrée) et `chataigne_diagnostics` les logs, avec
        `"readOnly": true`. **Étape 2 bis terminée.** Règle : Claude n'appelle
        que les outils de lecture (project, diagnostics, inspect, browse,
        observe…) ;
    - ✅ 26/10 étape 3 — vérifié par Claude via MCP (lecture seule) :
      module « Spatial » présent (`/modules/spatial`, catégorie DrawZone),
      il reçoit Fusion sur 12000 (~50 messages/s : `/object/update`,
      `/object/update/extra`, `/scene`, `/fusion`) ; 1 personne (id 1,
      x≈0.60, y≈0.34, immobile), scène 8 × 4,5 m. Aucune alerte.
      « Values » vide : normal, l'OSCModule de base ne crée pas de valeurs
      (c'est le module OSC générique qui a « Auto Add ») → à faire à
      l'étape suivante. « Log Incoming » activé = logs très chargés ;
  - ✅ 26/10 **fenêtre « Tracking Monitor »** (demande de Vincent, avant la
    suite) : voir en temps réel, sans log qui défile, les personnes reçues
    (id, coordonnées…) par le système d'entrée. Plan proposé, en attente
    de validation :
    - ✅ brique A — décoder Augmenta dans le module Spatial et tenir une
      liste de personnes **indépendante du protocole** (PSN/TUIO plus tard
      rempliront la même liste). Plan validé par Vincent (colonnes OK,
      brique C OK) :
      - ✅ code : `TrackingData.h/.cpp` (liste verrouillée, écrite par le
        fil réseau, lue par l'interface), `SpatialModule` décode `/scene`,
        `/object/enter|update|leave`, retire une personne sans nouvelles
        depuis 1 s, valeur « Person Count » dans Values ;
      - ✅ `.jucer` (+3 lignes, CRLF via `perl`) puis Projucer `--resave` :
        uniquement des ajouts ;
      - ❌ puis ✅ compilation : `build_dev.bat` ne se lance pas depuis le
        terminal de Claude (« not recognized », guillemets cmd imbriqués)
        → MSBuild appelé directement depuis PowerShell, mêmes options.
        Compilation 0 erreur (seul C4101 de Ben) ; ❌ édition de liens
        LNK1104 : `Chataigne.exe` verrouillé car Chataigne est ouvert ;
      - ✅ Chataigne fermé → édition de liens OK (exe du 26/10 19:55),
        Chataigne relancé en arrière-plan par Claude ;
      - ✅ test 1 : module recréé, `/mcp` Reconnect ; via MCP,
        `/modules/spatial/values/personCount` = 1 (lecture seule) avec la
        personne simulée de Fusion ;
      - ✅ test 2 (Vincent) : ajout / retrait de personnes simulées dans
        Fusion, « Person Count » suit ;
      - ✅ décision de Vincent : **source muette (> 1 s sans message) → la
        liste se vide**. Avant, le nettoyage ne tournait qu'à la réception
        de `/scene`, donc les personnes restaient si Fusion s'arrêtait.
        → `Timer` de 200 ms dans `SpatialModule` + `removeAllIfSilent()`.
        Compilation 0 erreur ; édition de liens en attente (Chataigne ouvert) ;
      - ✅ édition de liens OK (exe du 26/10 20:10), Chataigne relancé ;
      - ✅ test 3 (Vincent) : Fusion coupé → « Person Count » repasse à 0,
        Fusion relancé → le compte remonte. **Brique A validée.**
        Vincent préfère tester tout de suite plutôt que grouper avec la
        brique B : **étape par étape, pas d'écritures groupées** ;
    - ✅ brique B — panneau « Tracking Monitor » (menu des fenêtres de
      Chataigne) : en-tête source / état / messages/s / scène, puis tableau
      id, x, y (normalisé et mètres), âge, vitesse, rafraîchi ~10 fois/s ;
      - ✅ code : `spatial/ui/TrackingMonitorPanel.h/.cpp` (premier module
        Spatial trouvé, copie de la liste 10 fois/s puis dessin) ; includes
        dans nos fichiers `SpatialModule.h/.cpp` ; `Source/MainComponent.cpp`
        +2 lignes `// DrawZone:` (perl, fins de ligne conservées) ;
      - ❌ puis ✅ `FontOptions` (API JUCE 8) n'existe pas dans notre JUCE 7
        → `Font(taille)` comme dans le code de Ben (corrigé avant compilation) ;
      - ✅ `.jucer` (+6 lignes, sous-groupe `ui`) + Projucer : uniquement
        des ajouts ; compilation 0 erreur, liens en attente (Chataigne ouvert) ;
      - ✅ liens OK (exe du 26/10 20:19), Chataigne relancé ;
      - ✅ test (Vincent) : menu Windows → « Tracking Monitor » : en-tête,
        tableau qui suit Fusion, « Silent source » + liste vide quand Fusion
        est coupé. **Brique B validée.**
    - ✅ 26/10 commits **locaux** (accord de Vincent, rien poussé) sur
      `feature/spatial-module` : `bf1aaf99` chore (envois vers Ben coupés),
      `fdbf448c` feat(spatial) (module + briques A et B), `0380aaa5` docs ;
    - ✅ brique C — petite vue de dessus avec les points (validée par
      Vincent le 26/10) :
      - ✅ code : `paintTopView()` dans `TrackingMonitorPanel` (aucun
        nouveau fichier, pas de Projucer) : scène aux bonnes proportions,
        origine en haut à gauche, grille 1 m, graduations en mètres,
        un point coloré + id par personne ; vue au-dessus du tableau
        (60 % de la hauteur, le tableau garde au moins 5 lignes) ;
      - ✅ compilation 0 erreur ; liens OK (exe du 26/10 20:36),
        Chataigne relancé ;
      - ✅ test (Vincent) : points conformes à Fusion, déplacements dans le
        bon sens. **Axe Y vérifié : origine en haut à gauche, y vers le
        bas** (noté dans `augmenta-protocol.md`). **Brique C validée.**
      - ✅ commits locaux `5dccf7c6` feat(spatial) (vue de dessus) et
        `3f6be380` docs (rien poussé) ;
  - 💡→⬜ 26/10 demande de Vincent : **éditer les zones à la souris dans le
    Tracking Monitor** (glisser une zone = déplacer, 4 poignées aux angles
    = redimensionner). C'est le cœur de la v0.3 (« éditeur 2D ») de la
    roadmap ; proposé comme brique Z3 juste après Z2 ;
  - ⬜ réception Augmenta et liste des personnes ;
  - ⏭️ 26/10 une zone rectangle + booléen « présence ». Plan proposé, en
    attente de validation (modèle existant chez Ben : `FFTAnalyzerManager`
    du module Audio, liste d'éléments dans les paramètres, valeurs
    correspondantes dans Values) :
    - ✅ brique Z1 — liste « Zones » dans les paramètres du module (centre
      et taille en mètres) ; par zone, dans Values : « Presence » et
      « Person Count ». Liste validée par Vincent :
      - ✅ code : `SpatialZone.h/.cpp` (BaseItem), `zoneManager` dans
        `moduleParams` (sauvegardé avec la session), valeurs dans
        Values → Zones → <nom de zone> ; zones calculées sur le fil
        principal à 50 Hz (le minuteur passe de 200 ms à 20 ms) ;
      - ✅ `.jucer` + Projucer (ajouts seulement), compilation 0 erreur,
        liens OK (exe du 26/10 20:48), Chataigne relancé ;
      - ❌ Chataigne fermé par erreur juste après la relance (code 0, pas
        de plantage dans le log) → relancé ;
      - ✅ test (Vincent) : ajout de zone, présence, renommage, suppression ;
      - ✅ test sauvegarde / réouverture (Vincent) : la zone revient
        (`D:\DRAWZONE\ChataigneProject\TEST_ZONES.noisette`, nouveau
        dossier de Vincent pour les sessions de test, hors Git).
        **Brique Z1 validée.** Commit local `264f2b48` (rien poussé) ;
    - ✅ brique Z2 — zones dessinées dans la vue de dessus du Tracking
      Monitor (remplie quand présence) :
      - ✅ code : chaque zone a une couleur (« Color » de l'élément,
        aléatoire à la création) ; le panneau copie les zones 10 fois/s et
        les dessine sous les personnes (contour + nom, remplie si
        présence, grisée si désactivée). Compilation 0 erreur, liens OK
        (exe du 26/10 20:58), Chataigne relancé ;
      - ❌ test Vincent : **plantage** en changeant la taille de la zone.
        Lu par MCP : crash dans `paintTopView` → `drawText` ; taille de la
        zone = **22 369 624 m** (glisser sur le champ fait exploser la
        valeur, car j'avais mis une borne quasi infinie `INT32_MAX`) →
        rectangle de >1 milliard de pixels, le moteur de dessin de JUCE
        plante. Correction : bornes finies (centre ±1000 m, taille
        0–1000 m, `SpatialZone::maxMeters`) + zone découpée à la scène
        avant dessin ;
      - ❌ compilation : C1041, `Chataigne.pdb` verrouillé : Chataigne est
        encore ouvert dans son gestionnaire de crash (il lit le .pdb pour
        la pile d'appels) → fermer Chataigne puis recompiler ;
      - ✅ demande de Vincent : **les bornes d'une zone suivent la scène
        reçue** (Fusion 8 × 4,5 m → centre 0–8 / 0–4,5, taille ≤ 8 × 4,5).
        `SpatialZone::setSceneSize()`, appliqué quand `/scene` change de
        taille et à la création ; 1000 m tant que la scène n'est pas reçue ;
        une nouvelle zone démarre au centre de la scène. Effet de bord :
        si la scène rétrécit, les zones trop grandes sont recadrées ;
      - ❌ puis ✅ recompilation : MSBuild répond « à jour » sans rien
        recompiler (exe resté à 20:58) — la compilation ratée par C1041
        a faussé son suivi. → `.obj` du projet supprimés, recompilation
        complète en 3 min 26 (seuls avertissements : ceux de Ben, C4101 et
        C4206). Exe du 26/10 21:10, Chataigne relancé ;
      - ✅ test (Vincent) : plus de plantage, bornes = scène, zone dessinée
        et remplie, nouvelle zone au centre ;
      - ❌ éditeur graphique 2D de « Center » (Inspector) : y vers le haut,
        alors que la scène a y vers le bas (Screenshot_4). → option de
        Chataigne `extendedEditorInvertY` activée par défaut sur Center
        (= « Invert Y » du clic droit). Compilation 0 erreur, liens OK
        (exe du 26/10 21:17), Chataigne relancé ;
      - ✅ test Vincent : Invert Y OK. **Brique Z2 validée.**
      - ⬜ test Vincent ;
  - ⬜ test avec Augmenta Fusion, puis tag `v0.1.0`.

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
- Projucer : `D:\DRAWZONE\JUCE\extras\Projucer\Builds\VisualStudio2022\x64\Release\App\Projucer.exe`
- Procédure complète et dépannage : `docs/drawzone/BUILDING.md`.
