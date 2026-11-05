# Archive du TODO — DrawZone, Phases 0 à 2, v0.1 à v0.5, revue du 28/10, journal du 23 au 26/10

Sorti de `TODO.md` le 29/10/2026 (tâche d'optimisation 25) pour qu'il reste court.
Contenu inchangé, rien n'a été réécrit. v0.5 et revue du 28/10 ajoutées le 05/11/2026. Travail en cours : `TODO.md`.

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

## v0.1 to v0.4

- ✅ **v0.1 — squelette du module Spatial** (v0.1.0, 27/10/2026) :
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
  - ✅ module vide enregistré dans Chataigne, compilé et visible dans le menu :
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
    - ✅ test par Vincent : module visible dans le menu, réception OSC ;
  - ✅ **workflow de test « Claude lance Chataigne et lit les logs »** (24/10) :
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
    - ✅ étape 2 bis — **brancher Claude Code sur le serveur MCP de Chataigne**
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
      - ✅ Vincent relance Claude Code depuis `D:\DRAWZONE`
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
  - ✅ 26/10 brique Z3 (demande de Vincent) : **éditer les zones à la
    souris dans le Tracking Monitor** (glisser = déplacer, 4 poignées aux
    angles = redimensionner ; clic droit Duplicate / Delete / Hide, ajoutés
    en cours par Vincent). C'est le cœur de la v0.3 (« éditeur 2D ») :
    - ✅ code : test de clic (poignées puis intérieur, zone du dessus en
      premier), curseurs, poignées sur la zone survolée, un seul pas
      d'annulation par glissement (Ctrl+Z), zone sélectionnée dans
      l'Inspector au clic ; « Hide » = nouveau paramètre « Hidden » (la
      zone détecte toujours), « Show hidden zones » au clic droit dans le
      vide ; une zone dupliquée n'est plus recentrée ;
    - ✅ règle de Vincent ajoutée à `CLAUDE.md` : tout ce qui est visible
      dans Chataigne est en anglais ;
    - ✅ compilation 0 erreur (exe du 26/10 21:22), Chataigne relancé ;
    - ✅ test Vincent : déplacement, poignées, Ctrl+Z, Duplicate, Delete,
      Hide, Show hidden zones : tout fonctionne ;
    - ✅ retours de Vincent, codés : bouton « Show hidden zones (N) » sous
      la vue 2D (visible s'il y a des zones masquées) au lieu du clic
      droit ; clic droit dans la scène hors zone → « Create zone » (centrée
      sur le clic) ; vue 2D centrée et agrandie (mise en page calculée
      dans `resized()` : en-tête, vue, bouton, tableau de hauteur fixe
      6 personnes). Compilation 0 erreur, liens OK (exe du 26/10 21:30),
      Chataigne relancé ;
    - ✅ test Vincent : tout fonctionne. **Brique Z3 validée.**
  - ✅ réception Augmenta et liste des personnes (briques A, B, C) ;
  - ✅ 26/10 une zone rectangle + booléen « présence ». Plan proposé, en
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
  - ✅ 27/10 test final v0.1 avec Augmenta Fusion, puis tag `v0.1.0`.
    - ✅ Chataigne lancé par Claude (exe du 26/10 21:30), MCP à l'écoute sur 42002 ;
    - ✅ test Vincent (lu par MCP) : Action de State Machine sur « Presence », Log a message en TRUE / FALSE, alternance entrée / sortie correcte (02:33:45 → 02:33:49) ;
    - ✅ avertissement « Link is broken : /modules/spatial/values/zones/zoneDETEST/presence » (02:31:34, après un renommage de Vincent) : test refait, zone renommée « ZONE A » (lu par MCP), le lien tient, entrées / sorties toujours loguées (02:35 → 02:36), pas de nouvel avertissement. Non reproduit ;
    - ✅ session enregistrée (02:37, le fichier pointe bien sur `zoneA/presence`), Chataigne fermé proprement (code 0), relancé par Claude ;
    - ✅ test Vincent (lu par MCP) : session rouverte, pas d'avertissement, entrées / sorties loguées (02:38:04 → 02:38:08). **Test final v0.1 validé.**
    - ✅ `CHANGELOG.md` (section 0.1.0), `ROADMAP.md`, `CLAUDE.md` à jour ; fusion dans `develop` et tag annoté `v0.1.0` (local, rien poussé) ;
    - ❌ puis ✅ fusion : `git merge -F -` refusé (« could not read file '-' ») mais la commande suivante a posé le tag sur l'ancien `develop` → tag local supprimé, fusion refaite avec `-m`, tag `v0.1.0` sur le commit de fusion `53221961` ;

- ⏭️ 27/10 **styles d'UI du Tracking Monitor** (demande de Vincent, avant la v0.2) : 3 maquettes (A barre d'état + tableau enrichi, B grande vue + panneau latéral Zones / Personnes, C mode « show » plein écran) + l'actuel, en page de design : https://claude.ai/artifact/UGUXh7g1mcD76V19rrH928. Contrainte : ne changer que le dessin (`paint`, `resized`), pas les données ni les zones. → Vincent choisit : style et vue 2D de la C (sans les cartes du bas) + colonne Zones / People de la B. 2e tour : D1 colonne en liste, D2 colonne en cartes avec l'état de la source dedans, D3 rail repliable (même page, rangée « Round 2 »). → **choix de Vincent : D1 + le repli en rail de D3.** Découpage en briques proposé (U1 à U5). Vincent autorise à regrouper les briques d'UI : **lot 1 = U1 + U2 + U3** (style show, colonne D1, repli en rail), lot 2 = U4 + U5 ensuite. Branche `feature/tracking-monitor-ui` ;
  - ✅ lot 1 codé (`TrackingMonitorPanel` seulement : dessin, mise en page, bouton de repli, « + Create zone », « Show hidden ») ;
  - ❌ `build_dev.bat` ne se lance pas depuis le terminal de Claude (« 'C:\Program' is not recognized », souci de guillemets de cmd) → MSBuild appelé directement avec les mêmes options ;
  - ✅ compilation 0 erreur ; ❌ édition des liens LNK1104 : Chataigne ouvert (exe verrouillé), attendu → fermer Chataigne ;
  - ✅ Chataigne fermé par Vincent, liens OK (exe du 27/10 03:06), Chataigne relancé par Claude ;
  - ✅ test Vincent du lot 1 : OK. **Lot 1 validé.**
  - ⏭️ demande de Vincent : clic droit sur une zone → « Set color... » avec le sélecteur de couleur de Chataigne (`ColorParameterUI` sur la couleur de la zone, annulable par Ctrl+Z). Brique seule (touche à l'édition) ;
    - ✅ code : entrée « Set color... » (id 5) ; la zone est sélectionnée seule (le sélecteur modifie les éléments sélectionnés), un `ColorParameterUI` invisible au point du clic ouvre le sélecteur ; compilation 0 erreur, liens OK après fermeture par Vincent (exe du 27/10 03:12), Chataigne relancé ;
    - ✅ test Vincent : OK (« ok »). **Set color validé.**
  - ⏭️ demandes de Vincent, groupées (indépendantes, faciles à tester séparément) : clic gauche dans la vue hors zone → tout désélectionner ; flèche de direction sur les personnes en mouvement (vitesse mesurée dans le panneau à partir des positions, en m/s, lissée ; flèche si > 0,15 m/s, longueur = distance parcourue en 0,6 s, 14 à 60 px). Compilation 0 erreur, liens OK après fermeture par Vincent (exe du 27/10 03:17), Chataigne relancé ;
    - ✅ test Vincent : désélection et flèche OK (« tout fonctionne »). **Validé.**
  - ✅ commit, fusion dans `develop`, push de `develop` et du tag `v0.1.0` sur le fork (accord de Vincent) ;
  - ⏭️ 27/10 **lot 2 = U4 (traînée) + U5 (clic sur une zone de la colonne)**, branche `feature/tracking-monitor-ui-2` :
    - ✅ code : traînée des 2 dernières secondes de chaque personne (1 point / 100 ms, 20 points, s'estompe vers le plus ancien, sous les personnes) ; clic gauche sur une ligne de zone (colonne ou rail) → zone sélectionnée (Inspector). Compilation 0 erreur, liens OK (exe du 27/10 07:20), Chataigne relancé par Claude ;
    - ✅ test Vincent : traînée, clic dans la colonne et dans le rail OK (« tout fonctionne »). **Lot 2 validé.** Découpage U1 à U5 terminé ;
    - ✅ commit, fusion dans `develop` et push de `develop` sur le fork (accord de Vincent) ;

- ⏭️ 27/10 **v0.2**. Choix de Vincent : métriques d'abord, puis position relative ; position / distance = **personne la plus proche du centre** (la moyenne viendra en plus, plus tard) ; taux d'occupation = **personnes / capacité** (paramètre « Capacity » par zone, plafonné à 100 %). Découpage, une brique par test :
  - Tableau des valeurs présenté à Vincent ; ajouts validés : « Last Entered Id » (dans M1) ; zone vide → la position relative **garde sa dernière valeur** ;
  - ⏭️ M1 — déclencheurs « Entered » / « Exited » par zone, un par personne qui entre / sort (suivi des ids présents dans la zone, pas seulement du compte) + « Last Entered Id » (mis à jour juste avant « Entered ») :
    - ✅ code (`SpatialZone` seulement, branche `feature/zone-metrics`) ;
    - ✅ Chataigne fermé par Vincent, compilation 0 erreur, liens OK (exe du 27/10 07:35), Chataigne relancé par Claude ;
    - ✅ test Vincent : Entered / Exited / Last Entered Id OK. **M1 validé.** (Vincent demande que Claude relance Chataigne lui-même à chaque fois que c'est utile.)
  - ⏭️ M2 — « Presence Time » (s depuis que la zone est occupée, 0 quand vide) :
    - ✅ code (`SpatialZone`) : départ du chrono quand la zone passe de vide à occupée (liste des ids, pas la valeur Presence qui peut revenir d'une session enregistrée) ;
    - ✅ Chataigne fermé par Vincent, compilation 0 erreur, liens OK (exe du 27/10 07:40), Chataigne relancé par Claude ;
    - ✅ test Vincent : Presence Time OK. **M2 validé.**
  - ⏭️ M3 — « Capacity » + « Occupancy » (%) :
    - ✅ code (`SpatialZone`) : réglage « Capacity » (défaut 4, min 1, sous Center / Size) ; valeur « Occupancy » de 0 à 1 (= 0–100 %, barre dans Chataigne), plafonnée à 1 ;
    - ✅ Chataigne fermé par Vincent, compilation 0 erreur, liens OK (exe du 27/10 07:44) ; lancement interrompu, puis Chataigne relancé par Claude à la demande de Vincent ;
    - 💬 Vincent : « compilations de plus en plus longues ? » → non, c'est la modification de `SpatialZone.h` (inclus par ~10 gros fichiers du unity build) qui recompile beaucoup ; un `.cpp` seul recompile un seul fichier. Piste : déclarer d'avance dans le `.h` les valeurs des briques suivantes ;
    - ✅ test Vincent : Capacity / Occupancy OK. **M3 validé.**
  - ⏭️ P1 — position relative de la personne la plus proche du centre : « Relative Position » (0–1 dans la zone) et « Distance To Center » (m) :
    - ✅ code (`SpatialZone`) : valeurs gardées quand la zone se vide ; les pointeurs de P2 (« Average Position », « Average Distance To Center ») sont déjà déclarés dans le `.h` pour que P2 ne touche que le `.cpp` (compilation plus courte) ;
    - ✅ Chataigne fermé par Vincent, compilation 0 erreur, liens OK (exe du 27/10 07:50), Chataigne relancé par Claude ;
    - ✅ test Vincent : fonctionne, **mais changement demandé** : la position relative doit être rattachée à la **personne** (id du tracker), pas à la zone ; dans un recouvrement de zones, position relative dans la **zone de recouvrement**. Choix de Vincent : liste par id (Values → People → « Person 12 », créée / supprimée avec la personne) ; valeurs P1 de la zone retirées ; hors zone → **-1, -1** ;
  - ⏭️ P1b — Values → People → « Person <id> » : Id, Position (m), Zone (nom, « A + B » en recouvrement, vide hors zone), Relative Position (0–1 dans la zone ou dans l'intersection de toutes les zones qui la contiennent, -1 hors zone) :
    - ✅ code : classe `SpatialPerson` (dans `SpatialModule.h/.cpp`, pas de nouveau fichier donc pas de Projucer), mise à jour à 50 Hz après les zones ; « People » non enregistré dans la session ; Relative Position / Distance To Center retirés de `SpatialZone` ;
    - ✅ Chataigne fermé par Vincent, compilation 0 erreur, liens OK (exe du 27/10 08:01), Chataigne relancé par Claude ;
    - ✅ test Vincent : People, Zone, recouvrement, disparition OK. **P1b validé.**
  - ⏭️ P2 — moyenne du groupe, sur la zone : « Average Position » (barycentre des personnes présentes, 0–1 dans la zone) et « Average Distance To Center » (m, du barycentre au centre) ; gardées quand la zone se vide (choix de Vincent pour les valeurs de zone) :
    - ✅ code (`SpatialZone.cpp` seulement, pointeurs déjà déclarés) ;
    - ✅ demande de Vincent, groupée avec P2 (dessin seulement) : colonne « relative » dans le tableau People du Tracking Monitor, entre « x, y (m) » et « zone » (lue dans Values → People, « - » hors zone) ; colonne de droite élargie de 240 à 340 px (demande de Vincent : « élargir un peu pour que ça reste visible ») ;
    - ✅ Chataigne fermé par Vincent, compilation 0 erreur, liens OK (exe du 27/10 08:08), Chataigne relancé par Claude ;
    - ✅ test Vincent : Average Position / Distance et colonne « relative » OK. **P2 validé.** Métriques et positions terminées ;
  - ⏸️ 27/10 fin de séance. Branche `feature/zone-metrics` **non commitée** (M1, M2, M3, P1b, P2, colonne « relative »), en attente de l'accord de Vincent. Reprise : anti-clignotement ;
  - ✅ 27/10 reprise : accord de Vincent → commit de `feature/zone-metrics`, fusion dans `develop`, push de `develop` sur le fork ;
    - ❌ mise à jour de TODO.md par un script Python avant le commit : Python absent de la machine → TODO.md mis à jour ensuite avec l'outil d'édition (cette entrée), commit fait quand même ;
  - ⏭️ anti-clignotement, branche `feature/zone-antiflicker`. Choix de Vincent : **marge + délai de sortie** par zone :
    - ✅ code : « Exit Margin » (m, défaut 0,1 : une fois dedans, on ne sort qu'au-delà du bord + marge) ; « Exit Delay » (s, défaut 0,3 : une personne sortie ou perdue reste comptée pendant ce délai, si elle revient ni Exited ni Entered) ; Average Position calculée sur les personnes vraiment dedans ; People → Zone / Relative Position et Tracking Monitor suivent la même appartenance que la zone (Relative Position bornée à 0–1 dans la marge) ;
    - ✅ compilation 0 erreur, liens OK (exe du 27/10 12:33), Chataigne relancé par Claude ;
    - ✅ test Vincent : OK (« ok »). **Anti-clignotement validé.**
    - ✅ commit local sur `feature/zone-antiflicker` (accord de Vincent : « commit juste », sans fusion ni push) ;
  - ⏭️ forme **cercle**, branche `feature/zone-shapes`. Choix de Vincent : **ellipse via Size** (réglage « Shape » Rectangle / Ellipse ; l'ellipse remplit le rectangle Center + Size, cercle si largeur = hauteur ; Relative Position 0–1 dans le carré englobant) :
    - ✅ code : test d'appartenance elliptique (Exit Margin ajoutée aux demi-axes) ; Tracking Monitor : ellipse dessinée (halo, remplissage, contour), cadre fin + poignées aux coins quand elle est sélectionnée / survolée, clic dans l'ellipse seulement ;
    - ✅ compilation 0 erreur, liens OK (exe du 27/10 13:44), Chataigne relancé par Claude ;
    - ✅ test Vincent : OK (« ok »). **Ellipse validée.** Non commitée ;
  - ⏭️ demande de Vincent : double clic sur le nom d'une zone → renommer (même branche `feature/zone-shapes`) :
    - ✅ code (`TrackingMonitorPanel` seulement) : double clic sur une ligne de zone de la colonne (pas le rail, sans noms) ou sur le nom en haut à gauche d'une zone dans la vue → champ de texte sur place ; Entrée ou clic ailleurs = valider (annulable par Ctrl+Z), Échap = annuler, nom vide ignoré ;
    - ✅ Chataigne fermé par Vincent, compilation 0 erreur, liens OK (exe du 27/10 17:09), Chataigne relancé par Claude ;
    - ✅ test Vincent : « le rename fonctionne ». **Renommage validé.**
  - ⏭️ demande de Vincent : clic droit sur une zone de la colonne (et du rail) → même menu que dans la vue (Duplicate, Delete, Hide, Set color...) ; pour une zone cachée, « Show » à la place de « Hide » :
    - ✅ code (`TrackingMonitorPanel` seulement) : menu déplacé dans `showContextMenu()`, partagé par la vue et la colonne ;
    - ✅ Chataigne fermé par Vincent, compilation 0 erreur, liens OK (exe du 27/10 17:16), Chataigne relancé par Claude ;
    - ✅ test Vincent : OK (« ok »). **Menu clic droit de la colonne validé.**
  - ⏸️ 27/10 fin de séance. `feature/zone-antiflicker` commitée localement (non fusionnée, non poussée). `feature/zone-shapes` (partie d'elle) commitée localement (accord de Vincent : « commit juste ») : ellipse, renommage par double clic, menu clic droit dans la colonne. Reprise : polygone ;
  - ⏭️ demande de Vincent : que Claude puisse lui-même reconnecter le serveur MCP « chataigne » à chaque redémarrage de Chataigne (ECONNREFUSED au démarrage de Claude Code car Chataigne n'était pas lancé, et pas de reconnexion automatique ensuite) :
    - ❌ 27/10 soir : rien de prévu côté Claude Code pour qu'il se reconnecte seul (`/mcp` est une commande de l'utilisateur) ; pas de Node ni de Python sur la machine pour un relais local ;
    - ❌ premier essai curl : 404 puis « Session protocol version mismatch » → il faut le handshake `initialize` + l'en-tête `MCP-Protocol-Version: 2025-06-18` + `Mcp-Session-Id` ;
    - ✅ script `D:\DRAWZONE\tools\chataigne-mcp.sh` (hors du dépôt) : appelle directement le serveur MCP de Chataigne par curl, nouvelle session à chaque appel, donc marche après chaque redémarrage sans reconnexion. `list` = liste des outils ; `<outil> '<json>'` = appel. Testé avec `chataigne_diagnostics` : OK ;
  - ⏭️ forme libre (branche `feature/zone-shapes`). Choix de Vincent : option « Draw free shape », un clic par point, fermeture par clic sur le premier point ou double clic ; trois types de point : normal (Linear), Quadratic (une poignée), Cubic (deux poignées), à changer par clic droit sur le point. Deux briques :
    - ⏭️ F1 — forme libre à segments droits :
      - ✅ code : « Shape » → Free Shape ; points rangés dans la zone (Points → Point N → Position, 0–1 dans la boîte Center + Size, enregistrés dans la session) ; appartenance par `Path::contains` + Exit Margin par distance au contour ; passage en Free Shape depuis l'Inspector = 4 coins de la boîte. Tracking Monitor : clic droit sur la scène → « Draw free shape » (clic = point, clic sur le premier point / double clic / Entrée = fermer, Retour arrière = retirer le dernier point, Échap = annuler) ; poignées rondes sur les points, déplaçables (la boîte suit, un seul Ctrl+Z) ; poignées de coin = mise à l'échelle. Pointeurs des courbes (F2) déjà déclarés dans le `.h` ;
      - ❌ `build_dev.bat` lancé depuis le terminal de Claude : « not recognized » (déjà noté dans BUILDING.md §6) → MSBuild appelé directement ;
      - ✅ Chataigne fermé par Vincent, compilation 0 erreur, liens OK (exe du 27/10 20:22), Chataigne relancé par Claude ;
      - ✅ test Vincent : « OK pour les tests ». **F1 validée.**
    - ⏭️ D1 — demande de Vincent : clic droit sur la scène → Draw rectangle / Draw ellipse / Draw free shape (rectangle et ellipse : appuyer, glisser, relâcher) :
      - ✅ code (`TrackingMonitorPanel` seulement) : « Create zone » gardé, puis Draw rectangle / Draw ellipse / Draw free shape ; aperçu pendant le glissement, Échap = annuler, simple clic sans glisser = rien de créé (< 5 cm) ;
      - ✅ Chataigne fermé par Vincent, compilation 0 erreur, liens OK (exe du 27/10 20:31), Chataigne relancé par Claude ;
      - 💬 Vincent : ne pas envoyer de messages d'avancement pendant la compilation (« n'arrête pas la compilation pour me dire chaque étape ») → un seul message à la fin ;
      - ✅ test Vincent : « ok pour ça ». **D1 validée.**
    - ⏭️ F2 — clic droit sur un point : Linear / Quadratic / Cubic, poignées des courbes :
      - ✅ code : ZonePoint → Interpolation, Handle In, Handle Out (m, depuis le point) ; contour en courbes (cubique si les deux côtés ont une poignée, quadratique si un seul) ; Quadratic = poignée Out reflétée de l'autre côté. Tracking Monitor : clic droit sur un point → menu Linear / Quadratic / Cubic (un Ctrl+Z) ; au premier passage en courbe, poignées le long de la direction point précédent → suivant ; poignées (losanges) visibles et déplaçables sur la zone sélectionnée ; les poignées suivent la mise à l'échelle par les coins ; clic dans la forme testé sur la courbe (elle peut sortir de la boîte). Enregistré dans la session ;
      - ✅ Chataigne fermé par Vincent, compilation 0 erreur, liens OK (exe du 27/10 20:37), Chataigne relancé par Claude ;
      - ✅ test Vincent : « ok ». **F2 validée.** Formes terminées (rectangle, ellipse, forme libre à courbes) ;
    - ✅ commit local sur `feature/zone-shapes` (accord de Vincent : « commit juste », sans fusion ni push) ;
  - ⏸️ slots de joueurs : mis en attente par Vincent (27/10) pour régler d'abord des détails d'interface et de placement des informations ;
  - ⏭️ demande de Vincent : valeurs des zones en temps réel sous forme de tableau dans le Tracking Monitor, 4 propositions d'agencement à comparer (branche `feature/zone-info-layouts`, partie de `feature/zone-shapes`) :
    - ✅ code (`TrackingMonitorPanel` seulement, dessin) : bouton « Zone values : … » dans l'en-tête pour basculer entre 1. Cards (3 lignes par zone dans la colonne), 2. Column table (tableau compact dans la colonne), 3. Bottom table (tableau large sous la vue de dessus, toutes les valeurs), 4. On the zones (étiquette sous chaque zone + croix au barycentre) ;
    - ✅ choix de Vincent : garder les **Cards**, plus lisibles → 4 maquettes texte proposées avant le code (A grille, B une ligne par valeur, C compacte, D compacte dépliable) ; choix : **D avec le détail en B**. Et : clic sur une carte → Inspector = paramètres, puis Values, puis Points (en dernier, repliés) ;
    - ✅ code : les 3 autres agencements et le bouton de choix retirés (retour au fichier commité puis cartes D seules) ; carte = flèche de dépliage, pastille, nom, « n / cap · temps », barre d'occupation pleine largeur + % ; dépliée : Occupancy, Presence time, Last entered, Avg position, Avg distance (étiquette à gauche, valeur à droite) ; carte teintée de la couleur de la zone quand occupée. Inspector : `SpatialZoneEditor` (dans `SpatialZone.h/.cpp`) ajoute les Values de la zone (titre « Values ») avant les Points ; Points repliés par défaut ;
    - ❌ compilation : `Array<BaseItem*>(zone)` pris pour le constructeur « tableau terminé par null » (C2678) → `Array<BaseItem*>((BaseItem*)zone)` ;
    - ✅ Chataigne fermé par Claude, compilation 0 erreur, liens OK (exe du 27/10 21:01), Chataigne relancé par Claude ;
    - ✅ test Vincent : « ok ». **Cartes D + Inspector (Values, Points repliés) validés.** Non commité ;
    - 💬 Vincent : « c'est ok pour les informations et l'UI » → détails d'interface terminés, retour aux slots de joueurs ;
    - ✅ commit local sur `feature/zone-info-layouts` (accord de Vincent), sans fusion ni push ;
  - ⏭️ slots de joueurs, choix de Vincent : portée **scène** ; perte de tracking : **les deux au choix** (paramètre : libération immédiate ou délai de grâce avec reprise près de la dernière position) ; slots pleins : nouvelle personne **ignorée** (pas de file d'attente) ;
    - ✅ proposition validée (« ok ») : paramètres Player Slots (Enabled, Slot Count, Release Mode Immediate / Grace Delay, Grace Delay, Rejoin Distance) ; Values > Slots (Free Slots ; par slot : Occupied, Person Id, Position, Joined, Left) ; 2 briques : 1. logique + Values, 2. numéro de slot dans le Tracking Monitor ;
    - ✅ brique 1, code : `PlayerSlots.h/.cpp` (nouveaux fichiers, inclus par SpatialModule, pas de passage par le Projucer), branche `feature/player-slots` ; plus petit slot libre ; slots pleins → personne ignorée jusqu'à sa sortie de la scène ; en Grace Delay, slot réservé (position figée), reprise sans Left ni Joined par une nouvelle personne à moins de Rejoin Distance, ou par le même id qui revient ;
    - ✅ compilation 0 erreur (exe du 27/10 22:23), Chataigne relancé par Claude ;
    - ✅ (28/10) avant le commit : déclarer `PlayerSlots.h/.cpp` dans `Chataigne.jucer` (compile="0", comme les autres fichiers spatial) puis réenregistrer avec le Projucer ; → fait : 2 lignes dans `Chataigne.jucer`, `Projucer --resave` : seuls les projets Xcode (×2) et Visual Studio (×3, .vcxproj + .filters) changent, uniquement des lignes ajoutées ; recompilation OK (rien à refaire), Chataigne relancé ;
    - 💬 demandes de Vincent avant commit : modes renommés, gestion des slots dans le Tracking Monitor, slots renommables, nom du slot affiché sur la personne dans la vue 2D, emplacement réservé visible dans la vue 2D (plus tard en NDI/Spout) ;
    - ✅ proposition validée : modes **Next Person Takes Slot** (slot libéré aussitôt, pour la dernière personne arrivée) et **Hold Spot To Reclaim** (slot réservé à la dernière position ; la 1re personne sans slot qui entre dans le cercle Reclaim Radius le reprend ; Hold Time, 0 = sans limite) ; slots renommables avec une couleur ;
    - ✅ intégration UI : 4 propositions (A colonne, B onglets, C bandeau sous la vue, D fusion avec People) → choix **C** (bandeau de slots façon HUD sous la vue de dessus) ; retirer le gros chiffre (nombre de personnes) en bas à droite des zones dans la vue 2D ;
    - ✅ brique 1 (logique) réécrite : paramètre Slot Release, Hold Time, Reclaim Radius ; par slot : Color, Occupied, Person Id, Position, Held, Hold Remaining, Joined, Left ; personnes sans slot en file (ordre d'arrivée), un slot libéré va aux dernières arrivées ; noms et couleurs enregistrés dans le projet (clé « slots ») ;
    - ✅ compilation 0 erreur (exe du 27/10 22:54), Chataigne relancé par Claude ;
    - ✅ test Vincent : « ok pour les tests ». **Brique 1 (logique des slots) validée** ;
    - ✅ brique 2 (interface), code : bandeau Player Slots sous la vue de dessus (titre + « n / N free », une puce par slot : pastille pleine / anneau, nom, « #id » / « held 4.2 s » / « — », bordure en pointillés si held, boutons - / +) ; clic = Inspector, double clic = renommer, clic droit = Rename, Set color..., Enabled, choix du mode ; vue 2D : étiquette du slot (couleur du slot, nom + #id) qui suit la personne, cercle en pointillés de rayon Reclaim Radius à l'emplacement réservé avec nom, temps restant et arc qui se vide (∞ si Hold Time = 0) ; gros chiffre (nombre de personnes) retiré des zones ;
    - ❌ 1er essai de liens bloqué (LNK1104) : Chataigne attendait la réponse à « enregistrer ? » à la fermeture → Vincent a répondu, compilation terminée ;
    - ✅ compilation 0 erreur (exe du 27/10 23:05), Chataigne relancé par Claude ;
    - ✅ test Vincent : « ok pour les tests ». **Brique 2 (interface des slots) validée** ;
    - 💬 Vincent : les modes s'appellent « SwapSlots » entre nous ; le choix du mode doit être une liste déroulante dans l'UI, pas dans le clic droit ; clic sur une personne → tous ses paramètres et valeurs dans l'Inspector ;
    - ✅ code : liste déroulante du mode dans le bandeau (retirée du clic droit) ; clic sur une personne (vue de dessus, liste People, rail) → sélection (Inspector), personne surlignée ; Values > People > Person N reçoit Slot, Velocity, Orientation, Height, Age ; les slots sont calculés avant les personnes ;
    - ✅ compilation 0 erreur (exe du 27/10 23:16), Chataigne relancé par Claude ;
    - 💬 Vincent : liste déroulante « moche » → 4 propositions (A puce déroulante, B puce compacte avec icône + menu expliqué, C texte cliquable, D interrupteur) ; choix **B**, placée **tout à gauche** du bandeau ;
    - ✅ code : ComboBox JUCE remplacée par une puce dessinée (icône flèche en pointillés = Next person, cercle en pointillés = Hold spot ; nom court ; chevron ; survol éclairci) ; menu : nom complet + explication grise + coche ;
    - ✅ compilation 0 erreur (exe du 27/10 23:21), Chataigne relancé par Claude ;
    - 💬 Vincent : assigner vite un slot à un id depuis le Tracking Monitor → proposition : 3 gestes (glisser-déposer, clavier 1–9 / 0, clic droit) + règle d'échange ; validés. Et : une petite section « Shortcuts » qui se déplie au survol ou au clic, avec les raccourcis importants (pavé numérique pour les slots) ;
    - ✅ brique 1, code : `PlayerSlots::assignByHand` (échange si la personne avait un slot, sinon l'occupant perd le slot et attend ; Left / Joined déclenchés) et `releaseByHand` (la personne reste sans slot jusqu'à sa sortie de la scène ou une assignation à la main) ; clic droit sur une personne → Assign slot ▸ (slots colorés, état, touche) / Release slot ; personne sélectionnée : 1–9 (pavé numérique aussi) = slot N, 0 / Suppr / Retour arrière = libérer ; puce « Shortcuts » à droite de l'en-tête : liste dépliée au survol, épinglée par un clic (Player slots, Zones, Free shape drawing) ;
    - ✅ compilation 0 erreur (exe du 27/10 23:53), Chataigne relancé par Claude ;
    - ✅ test Vincent : « ok ». **Brique 1 (assignation à la main, clavier, clic droit, Shortcuts) validée** ;
    - ✅ brique 2, code : glisser-déposer d'une personne (point de la vue, ligne People, rail) sur une puce de slot, ou d'une puce sur une personne ; ligne en pointillés de la couleur du slot jusqu'à la souris, cible surlignée, étiquette « Person N -> Slot » ; même règle d'échange (`assignSlot`) ;
    - ✅ compilation 0 erreur (exe du 27/10 23:59), Chataigne relancé par Claude ;
    - ✅ test Vincent : « ok ». **Glisser-déposer validé.** Slots de joueurs terminés (branche `feature/player-slots`) ;
    - ✅ commit local sur `feature/player-slots` (accord de Vincent, 28/10), sans fusion ni push ;
    - ✅ fusion locale dans `develop` (accord de Vincent, 28/10) : `--no-ff`, commit `72035834`, intègre toute la pile (zone-antiflicker, zone-shapes, zone-info-layouts, player-slots) ; sans push ;
    - ✅ CHANGELOG (entrée 0.2.0), ROADMAP (états v0.2 / v0.3 / v0.4, slots par zone reportés) et CLAUDE.md (état v0.2.0, prochaine étape v0.4) ; commit `docs` sur `develop` et tag annoté `v0.2.0` en local (accord de Vincent, 28/10), sans push ;
    - ✅ push vers `origin` (fork, accord de Vincent, 28/10) : `develop` (31a17e7c..0e3f7e27), les 4 branches `feature/*` de la v0.2 et le tag `v0.2.0`. **v0.2.0 publiée.** `upstream` non touché ;
  - 💬 Vincent (28/10) : NDI / Spout en **v0.4**, le reste décalé → ROADMAP (tableau + section v0.4), README du fork, CLAUDE.md (prochaine étape, émetteur PSN en v0.5), DECISIONS (D13) et l'étape de test multi-scènes / plusieurs systèmes placée avec le viewport 3D (v0.6, demande de Vincent) ; non commité ;
  - ✅ **v0.4 — sortie vidéo NDI / Spout** (branche `feature/video-output`, depuis `develop`) :
    - ✅ étude : Chataigne a déjà l'émetteur Spout (`juce_sharedtexture`, utilisé par la sortie de la timeline vidéo) et un module « NDI out » (bibliothèque NDI chargée à l'exécution, envoi sur un thread séparé ; `Processing.NDI.Lib.x64.dll` déjà dans `Binaries/App`) ;
    - ✅ proposition validée : image propre de la vue de dessus calculée par le module (marche Tracking Monitor fermé), bloc « Video Output » (Spout, NDI, Stream Name, FPS, fond, éléments affichés) ; échelle : **Output Area** = Scene (défaut, image = la scène sans bord, taille = scène × Pixels Per Meter, 100 px/m), Custom Area (Origin + Size en m), Fixed Resolution (scène centrée avec bandes) ; formule pixel = (x − origin) × px/m ; tests de réception dans **SMODE** ;
    - ✅ brique 1, code : dessin de la vue de dessus extrait dans `ui/TopViewRenderer.h/.cpp` (`TopViewModel` = données copiées du module + mouvements ; `TopViewRenderer::paint` avec options grille, noms, ids, traînées, slots, emplacements réservés, sélection) ; le Tracking Monitor l'utilise et ajoute ses poignées, ses aperçus de dessin et les graduations ; style déplacé dans le même fichier ; fichiers déclarés dans le `.jucer` (Projucer : lignes ajoutées seulement) ;
    - ✅ compilation 0 erreur (exe du 28/10 00:42), Chataigne relancé par Claude ;
    - ✅ test Vincent : « ok comme avant ». **Brique 1 validée** ;
    - ✅ étude Spout : l'émetteur de Chataigne envoie depuis le contexte OpenGL de l'interface (réglage « Use OpenGL Renderer », actif par défaut), qui ne dessine qu'aux rafraîchissements → la sortie demande une image à chaque tick (`triggerRepaint`) ;
    - ✅ brique 2, code : `SpatialVideoOutput.h/.cpp` (bloc « Video Output » des paramètres du module : Spout, Stream Name, Output Area Scene / Custom Area / Fixed Resolution, Pixels Per Meter, Area Origin, Area Size, Width, Height, FPS, Resolution en lecture seule) ; image calculée sur le thread principal à FPS avec `TopViewRenderer`, recopiée par l'émetteur Spout ; taille limitée à 8192 px ; fichiers déclarés dans le `.jucer` ;
    - ✅ compilation 0 erreur (exe du 28/10 00:47), Chataigne relancé par Claude ;
    - ❌ test Vincent dans SMODE : deux sources Spout, « DrawZone » noire et « SpoutLibrary » qui montre bien la vue. Cause probable : l'émetteur partagé de Chataigne ouvre un émetteur Spout 2.006 (`CreateSender`) qui ne reçoit jamais l'image, et l'envoi 2.007 (`SendTexture`) en ouvre un second sous le nom par défaut de la bibliothèque → correction dans notre code (le module de Ben est un sous-module, non modifié) : juste avant l'envoi, si le nom de l'émetteur Spout n'est pas le nôtre, on le libère et on lui donne notre nom (message « [DrawZone] Spout sender ... renamed » dans les logs) ;
    - ✅ compilation 0 erreur (exe du 28/10 01:00), Chataigne relancé par Claude ;
    - ❌ test Vincent : 4 sources (DrawZone, DrawZone_1, SpoutLibrary, SpoutLibrary_1) ; logs : 200 renommages « DrawZone_1 → DrawZone » en boucle. Cause : `ReleaseSender` ne ferme pas l'émetteur 2.006 ouvert par `CreateSender`, donc le nom « DrawZone » reste pris et Spout ajoute « _1 » → correctif retiré ;
    - ✅ nouvelle approche : la sortie n'utilise plus l'émetteur partagé de Chataigne mais **sa propre instance de la bibliothèque Spout** (`GetSpout()`) : `SetSenderName` une fois puis `SendImage` (pixels BGRA de l'image) sur le thread OpenGL de l'interface (`executeOnGLThread`), une image à la fois ; libération sur le thread OpenGL ; ❌ compilation : `juce::gl::GL_BGRA_EXT` est une macro des en-têtes Spout → valeur 0x80E1 ;
    - ✅ compilation 0 erreur (exe du 28/10 01:07), Chataigne relancé par Claude ;
    - ✅ test Vincent dans SMODE : « OK », une seule source DrawZone. **Brique 2 (Spout) validée** ;
    - 💬 Vincent : case à cocher « Spout Output » dans l'en-tête du Tracking Monitor, au style de l'interface ; clic sur « Spout Output » → paramètres dans l'Inspector ;
    - ✅ code : puce « Spout Output » dans l'en-tête, à gauche de « Shortcuts » (case cochée bleue avec coche quand la sortie est active, bordure bleue, survol éclairci) ; clic sur la case = activer / désactiver (annulable), clic sur le libellé = bloc Video Output dans l'Inspector ;
    - ✅ compilation 0 erreur (exe du 28/10 01:14), Chataigne relancé par Claude ;
    - ✅ test Vincent : « ok ». **Intégration Spout clôturée** (28/10) : commit local sur `feature/video-output` (sans fusion ni push) ;
    - ⬜ prochaine séance : brique 3 = sortie **NDI** (reprendre le chargement de la bibliothèque et le thread d'envoi du module « NDI out » de Chataigne), puis brique 4 = options d'affichage (grille, noms, ids, traînées, slots, emplacements réservés, fond transparent) ; ensuite fusion dans `develop`, CHANGELOG et tag `v0.4.0`, avec l'accord de Vincent ;
    - ✅ brique 3, code : case « NDI » dans le bloc Video Output (même Stream Name et FPS que Spout) ; bibliothèque NDI chargée comme le module « NDI out » de Chataigne (`VideoNetworkRuntime`) ; envoi sur un thread à part (`NDISender`) qui prend la dernière image, rien n'est envoyé tant que personne ne reçoit ; erreur (runtime absent) affichée dans Resolution ;
    - ✅ compilation 0 erreur (exe du 28/10 10:25), Chataigne relancé par Claude ;
    - ✅ test Vincent : « ok ». **Brique 3 (NDI) validée** ;
    - 💬 Vincent : ajouter NDI dans l'interface, comme Spout ;
    - ✅ code : puce « NDI Output » dans l'en-tête, entre « Spout Output » et « Shortcuts », même dessin et mêmes clics (case = activer / désactiver, libellé = bloc Video Output dans l'Inspector) ; dessin et clic des deux puces mis en commun (`paintOutputChip`, `handleOutputChipClick`) ;
    - ✅ compilation 0 erreur (exe du 28/10 10:51), Chataigne relancé par Claude ;
    - ✅ test Vincent : « ok ». **Puce NDI Output validée** ;
    - ✅ commit local `e86c85f2` sur `feature/video-output` (accord de Vincent, 28/10), sans fusion ni push ;
    - ✅ brique 4, code (proposition validée) : sous-bloc « Display » du bloc Video Output : Grid, Zone Names, Person Ids, Trails, Slot Labels, Held Spots, Background (noir par défaut, une seule couleur dans et hors de la scène), Transparent Background (Background grisé) ; image envoyée seulement, Tracking Monitor inchangé ; fond transparent : pixels « dé-prémultipliés » (JUCE stocke l'alpha prémultiplié, Spout et NDI attendent l'alpha droit), NDI en BGRA au lieu de BGRX ;
    - ✅ compilation 0 erreur (exe du 28/10 12:25), Chataigne relancé par Claude ;
    - ✅ test Vincent : « super ok ». **Brique 4 (options d'affichage) validée** ;
    - ✅ commit local `9d648b24` sur `feature/video-output` puis fusion locale dans `develop` (`--no-ff`, commit `09dd32dd`) (accord de Vincent, 28/10) ;
    - ✅ CHANGELOG (entrée 0.4.0), ROADMAP (v0.4 terminée) et CLAUDE.md (état v0.4.0, prochaine étape v0.5) ; commit `docs` sur `develop` et tag annoté `v0.4.0` ;
    - ✅ push vers `origin` (fork, accord de Vincent, 28/10) : `develop`, `feature/video-output` et le tag `v0.4.0`. **v0.4.0 publiée.** `upstream` non touché ;
    - ❌ (étape précédente) édition des liens bloquée (LNK1104) : Chataigne encore ouvert → relancée automatiquement après sa fermeture ;
    - ✅ compilation 0 erreur, liens OK (exe du 27/10 20:49), Chataigne relancé par Claude ;
    - 💬 Vincent (2e fois) : aucun message d'étape pendant la compilation → mémoire renforcée : la compilation attend toute seule la fermeture de Chataigne, un seul message final ;

## v0.5 (terminée le 31/10/2026)

Sorti de `TODO.md` le 05/11/2026, contenu inchangé.

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


## Étape d'optimisation, revue du 28/10/2026 (terminée)

Sorti de `TODO.md` le 05/11/2026, contenu inchangé.

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

---

## Journal de bord du 23 au 26/10/2026 (ex-`JOURNAL.md`, versé ici le 06/11/2026, contenu inchangé)


> **Figé le 29/10/2026** (tâche d'optimisation 24) : il faisait doublon avec
> `TODO.md`, qui garde désormais seul la trace de chaque séance (versions
> terminées : `docs/drawzone/TODO-archive.md`). Ce journal reste pour l'historique
> du 23 au 26/10/2026.

Résumé de chaque séance de travail : ce qui a été fait, les problèmes
rencontrés, où on s'est arrêté.

---

### 26/10/2026 — MCP branché, réception Augmenta et Tracking Monitor

- MCP : jeton lu dans le presse-papier (après 3 échecs avec `setx`),
  `.mcp.json` dans `D:\DRAWZONE` (hors dépôt, jeton non écrit). Claude Code
  relié à Chataigne en **lecture seule** (profil Inspect, `"readOnly": true`).
  Raté : 1re connexion refusée car Chataigne n'était pas lancé.
- Module Spatial vide testé avec Fusion : il reçoit ~50 messages/s.
- Demande de Vincent : une fenêtre de monitoring plutôt que des logs qui
  défilent. Plan en 3 briques validé :
  - **A** : décodage Augmenta (`/scene`, `/object/enter|update|leave`) dans
    une liste de personnes indépendante du protocole (`TrackingData`),
    valeur « Person Count », liste vidée quand la source est muette
    (> 1 s, choix de Vincent). Validée ;
  - **B** : panneau « Tracking Monitor » (menu Windows), en-tête d'état et
    tableau rafraîchis 10 fois/s. Validé ;
  - **C** : vue de dessus — prochaine étape.
- Ratés : `build_dev.bat` ne se lance pas depuis le terminal de Claude
  (MSBuild appelé directement), `FontOptions` (JUCE 8) absent de notre
  JUCE 7. Tous notés dans `TODO.md`.
- Règle confirmée par Vincent : une brique = un cycle compilation → test,
  jamais deux changements testés ensemble.
- Suite de la soirée :
  - vue de dessus dans le Tracking Monitor (axes Augmenta vérifiés :
    origine en haut à gauche, y vers le bas) ;
  - **zones rectangulaires** (liste « Zones », valeurs Presence et Person
    Count par zone, sauvegardées avec la session), dessinées dans le
    moniteur, bornes calées sur la scène reçue ;
  - **édition à la souris** dans le moniteur : déplacer, poignées,
    Ctrl+Z, clic droit Create / Duplicate / Delete / Hide, bouton « Show
    hidden zones ».
- Ratés : plantage du moniteur avec une zone de 22 millions de mètres
  (borne infinie) → bornes = scène + dessin découpé ; compilation qui
  croyait tout à jour après un échec → recompilation complète.
- Règles ajoutées : tout ce qui est visible dans Chataigne en anglais ;
  sessions de test dans `D:\DRAWZONE\ChataigneProject\` ; messages de
  Claude plus courts.
- Idées notées : moniteur plus « user friendly », vue 2D en Spout / NDI.
- Arrêt : tout est committé en local sur `feature/spatial-module`, rien
  poussé. Reprise : objectif v0.1 atteint (booléen « présence ») → bilan
  v0.1, puis tag `v0.1.0` à décider avec Vincent.

---

### 24/10/2026 — Première session dans VS Code avec Claude Code

- Lecture de `CLAUDE.md`, `JOURNAL.md`, `ROADMAP.md` et `TODO.md`, résumé du
  projet validé par Vincent.
- Petit raté : `TODO.md` n'a pas été trouvé à la première lecture, car il a été
  ajouté au dépôt pendant la séance. Relu ensuite.
- `TODO.md` devient le **document de traçabilité** du projet (réussites,
  échecs et corrections, idées). Section « Idées » ajoutée, règle ajoutée
  dans `CLAUDE.md`.
- `D:\DRAWZONE\DrawZone_old` : déjà supprimé.
- GitHub Actions : la CI de Ben tournait encore (run déclenché par notre
  push) → désactivée par Vincent. Push bloqué (`NE_PAS_PUSHER`) sur les
  sous-modules et JUCE. Règles « ne pas déranger Ben » dans `CLAUDE.md`.
- Vocabulaire : « le repo » = `D:\DRAWZONE` ; captures dans
  `D:\DRAWZONE\screenshots\` (hors Git).
- v0.1 démarrée :
  - protocole Augmenta v2 documenté (ordre des arguments pris dans le
    plugin officiel Unreal) ; décision **D12** : `SpatialModule` hérite de
    `OSCModule` ; idée « slots de joueurs » placée en v0.2 ;
  - branche `feature/spatial-module` : module Spatial vide (catégorie
    « DrawZone »), enregistré, `.jucer` réenregistré, compilé (0 erreur) ;
  - découverte : notre build envoyait des données aux serveurs de Ben
    (mises à jour, rapports de plantage, statistiques Matomo, conversion
    de fichiers) → bloc `// DrawZone:` dans `Source/Main.cpp`, compilé.
- Workflow de test : Claude lance Chataigne et lit les logs (fichier
  « Save logs » OK). Branchement MCP sur le serveur de Ben en cours :
  serveur OK, **jeton pas encore enregistré** (voir `TODO.md`).
- Ratés : `sed` qui convertit les fins de ligne du `.jucer`, outil
  d'édition qui uniformise `Main.cpp`, Projucer `--resave` qui rend la main
  trop tôt, consigne `setx` ambiguë. Tous notés dans `TODO.md`.
- Arrêt de séance : rien de la v0.1 n'est committé (branche
  `feature/spatial-module`, modifications locales). Reprise : jeton MCP,
  puis test du module Spatial vide.

---

### 23/10/2026 — Conception et v0.0 (dans claude.ai, avant VS Code)

#### Conception

- Idée de départ : un outil pour dessiner des zones d'interaction sur des
  surfaces, alimenté par des systèmes de tracking (Augmenta, Naostage,
  ZacTrack) via OSC, PSN, TUIO, avec une logique conditionnelle et des
  sorties OSC (booléens, pourcentages, durées).
- Évolution : logique « façon game design », inspirée des Blueprints d'Unreal.
- Choix de partir de Chataigne (voir DECISIONS.md D1, D2), Windows d'abord,
  2D avant 3D.
- Nom de projet : **DrawZone** (provisoire).

#### Installation (v0.0)

1. Build Tools VS 2022 installés (le lien `aka.ms` ne répondait pas dans
   Firefox → page « older downloads » / winget).
2. Fork GitHub `vincethpiafff/DrawZone`, clone de JUCE `develop-local`.
3. **Premier échec de compilation** : `JucePrecompiledHeader_Release.cpp`
   introuvable. Cause : le projet VS de Ben est généré pour sa machine
   (`D:\Projects\Dev\JUCE`) et les en-têtes précompilés ne sont pas versionnés.
   → Compilation du Projucer.
4. **Projucer bloqué** : « Incompatible License and Splash Screen Setting ».
   → Lecture du code source du Projucer : recompilation avec
   `JUCER_ENABLE_GPL_MODE=1`.
5. **1000 lignes parasites** dans `BinaryData` après enregistrement. Cause :
   conversion LF → CRLF de Git for Windows. → Ancien clone renommé
   `DrawZone_old`, nouveau clone avec `core.autocrlf=false`. Ensuite, un seul
   fichier modifié : le `.vcxproj`.
6. **Deuxième échec** : `WebView2.h` introuvable (paquet NuGet non restauré
   sans IDE). → Copie du paquet déjà présent dans `Builds/VisualStudio2022_CI/packages`
   (même version 1.0.902.49).
7. **Compilation réussie** (0 erreur, 2 avertissements du code de Ben).
8. **DLL manquantes** au lancement (JoyShockLibrary, Kinect20, Servus, SDL2).
   → Extraction du pack de dépendances de Ben à côté de l'exe.
9. **Chataigne démarre.** Messages `DNSServiceDiscovery error -65563` sans
   conséquence (Bonjour absent).
10. **Test Augmenta Fusion réussi** : messages reçus sur le module OSC, port 12000.
11. Commit `3d851a53` sur `develop`, tag `v0.0.0`, poussés sur GitHub.

#### Fin de séance

- Rédaction de la documentation de transfert (ce dossier, `CLAUDE.md`,
  `.github/README.md`) et du script `Tools/drawzone/setup_windows.bat`.
- Reste à faire :
  - vérifier que les GitHub Actions sont désactivées sur le fork ;
  - supprimer `D:\DRAWZONE\DrawZone_old` (plus utile) ;
  - démarrer la v0.1 dans VS Code avec Claude Code.
