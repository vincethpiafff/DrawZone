# Journal de bord (figé)

> **Figé le 29/10/2026** (tâche d'optimisation 24) : il faisait doublon avec
> `TODO.md`, qui garde désormais seul la trace de chaque séance (versions
> terminées : `docs/drawzone/TODO-archive.md`). Ce journal reste pour l'historique
> du 23 au 26/10/2026.

Résumé de chaque séance de travail : ce qui a été fait, les problèmes
rencontrés, où on s'est arrêté.

---

## 26/10/2026 — MCP branché, réception Augmenta et Tracking Monitor

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

## 24/10/2026 — Première session dans VS Code avec Claude Code

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

## 23/10/2026 — Conception et v0.0 (dans claude.ai, avant VS Code)

### Conception

- Idée de départ : un outil pour dessiner des zones d'interaction sur des
  surfaces, alimenté par des systèmes de tracking (Augmenta, Naostage,
  ZacTrack) via OSC, PSN, TUIO, avec une logique conditionnelle et des
  sorties OSC (booléens, pourcentages, durées).
- Évolution : logique « façon game design », inspirée des Blueprints d'Unreal.
- Choix de partir de Chataigne (voir DECISIONS.md D1, D2), Windows d'abord,
  2D avant 3D.
- Nom de projet : **DrawZone** (provisoire).

### Installation (v0.0)

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

### Fin de séance

- Rédaction de la documentation de transfert (ce dossier, `CLAUDE.md`,
  `.github/README.md`) et du script `Tools/drawzone/setup_windows.bat`.
- Reste à faire :
  - vérifier que les GitHub Actions sont désactivées sur le fork ;
  - supprimer `D:\DRAWZONE\DrawZone_old` (plus utile) ;
  - démarrer la v0.1 dans VS Code avec Claude Code.
