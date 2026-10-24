# Journal de bord

Résumé de chaque séance de travail : ce qui a été fait, les problèmes
rencontrés, où on s'est arrêté. Ajouter les séances les plus récentes en haut.

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
