# CLAUDE.md — DrawZone

Détails : `docs/drawzone/` (ROADMAP, DECISIONS, BUILDING, CONTRIBUTING, notes/).

## Projet

Outil open source (GPL-3.0), **fork de Chataigne** (Ben Kuper, C++/JUCE +
organicUI) : reçoit du tracking (Augmenta en OSC, PSN), dessine des zones
(2D vue de dessus, puis surfaces en 3D), logique façon Blueprints, sorties OSC.
Cœur : module **Spatial** dans le code, **Cockpit** à l'écran (D14), code dans
`Source/Module/modules/spatial/` ; ses zones exposent des paramètres Chataigne.

État : **v0.7.2** (07/11/2026 : Logic Graph, variables et OSC ; v0.7.1 : Live Mode). Suite : **v0.8**, graphe avancé.

## Travailler avec Vincent

- Répondre en **français**. Code, fichiers, commits, commentaires **et tout ce qui
  est visible dans Chataigne** : en **anglais**.
- Vincent n'est pas développeur C++ (il fait du Python) : expliquer simplement.
- Il valide chaque étape ; une liste de tâches qu'il a validée s'enchaîne jusqu'au
  bout, arrêt et discussion à la moindre erreur. Tests en simulation **jamais systématiques** :
  seulement pour intégrer une feature qui touche positions, zones, slots ou PSN ; sinon compiler, relancer, Vincent teste.
- Vérifier avant d'affirmer (lire le code, compiler, tester). Petites briques testables.
- `TODO.md` : le lire en début de séance, le tenir à jour **en une ligne par étape**
  (✅ ❌ avec cause, ⏭️ ⬜ 💡). Réponses et docs courtes : moins de tokens.
- Pas de commit ni de push sans l'accord de Vincent.

## Compiler, lancer, tester

```
D:\0-DRAWZONE\JUCE      JUCE 7.0.12 de Ben (develop-local), hors dépôt : jamais dans le dépôt
D:\0-DRAWZONE\DrawZone  ce dépôt (github.com/vincethpiafff/DrawZone)
```

- `build_dev.bat` (Release rapide, incrémental) puis `start "" "Binaries\App\Chataigne.exe"`.
  MSBuild : `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe`.
  Build Tools seuls, pas d'IDE. Champ ajouté / retiré dans un en-tête → `/t:Rebuild`.
- Nouveau clone : `Tools\drawzone\setup_windows.bat`. « DNSServiceDiscovery -65563 » : sans conséquence.
  `run_debug.cmd` et `Tools/vscode/debug.ps1` de Ben ne marchent pas chez nous.
- Nouveau fichier source : l'ajouter à `Chataigne.jucer`, puis
  `D:\0-DRAWZONE\JUCE\extras\Projucer\Builds\VisualStudio2022\x64\Release\App\Projucer.exe --resave Chataigne.jucer`
  (Projucer en mode GPL) ; `git status` : seuls les fichiers attendus changent.
- Unity build : `.cpp` en `compile="0"`, inclus dans `Source/Module/ModuleIncludes*.cpp`,
  en-têtes dans `ModuleIncludes.h`, modules enregistrés dans `ModuleFactory.cpp`.
- Tests : Augmenta Fusion → `127.0.0.1:12000` ; simulateurs `Tools/drawzone/sim/`
  (`run_checks.ps1`) ; protocole : `docs/drawzone/notes/augmenta-protocol.md`.

## Git

- Branches, commits (Conventional Commits en anglais), versions, fichiers à ne jamais commiter :
  `docs/drawzone/CONTRIBUTING.md`. `master` = Chataigne de Ben : ne pas toucher.
- `upstream` (Ben) et sous-modules `Modules/juce_*` et `D:\0-DRAWZONE\JUCE` : push
  bloqué (`NE_PAS_PUSHER`), ne jamais changer ; à refaire après un clone.
- Ne pas déranger Ben : jamais de PR vers `benkuper/Chataigne` (base = `vincethpiafff/DrawZone`),
  jamais `@benkuper` ni `benkuper/<dépôt>#<n°>`, GitHub Actions désactivées sur le fork.
  Le bloc `// DrawZone:` de `Source/Main.cpp` coupe tout envoi aux serveurs de Ben : le garder.
- `core.autocrlf=false` (dépôt et sous-modules). Fichiers de Ben en CRLF / mixtes :
  `git diff --stat` ne doit montrer que nos lignes ; pas de `sed` Git Bash sur eux,
  sinon `perl -0777 -pi`.

## Code de Ben

- Notre code dans nos fichiers ; chez Ben, accroches minimales marquées `// DrawZone:`,
  sans reformater. À nous : `CLAUDE.md`, `.github/README.md`, `docs/drawzone/`, `Tools/drawzone/`.
- Seule divergence : `Builds/VisualStudio2022/Chataigne_App.vcxproj` (notre chemin JUCE) ;
  en conflit : version de Ben puis `--resave`.
- Ne pas suivre : `AGENTS.md` (ni sa procédure de release), `MCP PLAN.md`, `Tools/mcp/`,
  `.github/workflows/build.yml` (doit rester désactivé).
