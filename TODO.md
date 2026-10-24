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
  - ⬜ lire la spécification Augmenta (`docs/drawzone/notes/augmenta-protocol.md`
    et le wiki Augmenta) ;
  - ⬜ étudier les modules PosiStageNet et OSC de Chataigne (unity build,
    `ModuleFactory.cpp`, `ModuleIncludes*.cpp`) ;
  - ⬜ décider : récepteur OSC de Chataigne réutilisé, ou port propre au module ;
  - ⬜ créer la branche `feature/spatial-module` ;
  - ⬜ module vide enregistré dans Chataigne, compilé et visible dans le menu ;
  - ⬜ réception Augmenta et liste des personnes ;
  - ⬜ une zone rectangle + booléen « présence » ;
  - ⬜ test avec Augmenta Fusion, puis tag `v0.1.0`.

## 💡 Idées

Idées notées en route, datées, à trier plus tard (roadmap, décision ou abandon).
Format : `- 💡 JJ/MM/AAAA — idée (contexte, qui l'a proposée)`.

- *(aucune pour l'instant)*

## Rappels

- Une étape à la fois, avec la validation de Vincent. Pas de commit sans son accord.
- Compiler : `build_dev.bat` · Lancer : `start "" "Binaries\App\Chataigne.exe"`
- Projucer : `D:\DRAWZONE\JUCE\extras\Projucer\Builds\VisualStudio2022\x64\Release\App\Projucer.exe`
- Procédure complète et dépannage : `docs/drawzone/BUILDING.md`.
