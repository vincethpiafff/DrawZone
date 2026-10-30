# CLAUDE.md — contexte du projet DrawZone

Ce fichier est lu automatiquement par Claude Code au début de chaque session.
Il résume tout ce qui a été décidé avant le passage dans VS Code. Les détails
sont dans `docs/drawzone/`.

## 1. Le projet en bref

DrawZone est un outil open source (GPL-3.0) pour les concepteurs d'expériences
immersives et interactives. Il doit :

1. recevoir des positions de personnes trackées (Augmenta, Naostage, ZacTrack…)
   via OSC et PosiStageNet (PSN) ;
2. permettre de dessiner des zones d'interaction, d'abord en 2D vue de dessus,
   puis sur des surfaces dans un viewport 3D ;
3. définir une logique « façon game design », inspirée des Blueprints d'Unreal
   (événements d'entrée/sortie, Branch, Delay, variables, machines à états) ;
4. envoyer des valeurs en sortie, en OSC en priorité : booléens, pourcentages,
   durées, compteurs.

DrawZone est un **fork de Chataigne** (Ben Kuper, C++/JUCE + organicUI).
On réutilise son interface, ses protocoles, sa State Machine, son Module Router,
Parrot (enregistrement/lecture), etc. Le cœur de DrawZone sera un nouveau type
de module **« Spatial »** (nom affiché : **Cockpit**, voir D14) dont les zones exposent leurs valeurs comme paramètres
Chataigne, donc utilisables partout dans le logiciel.

Roadmap complète : `docs/drawzone/ROADMAP.md`. Décisions et leurs raisons :
`docs/drawzone/DECISIONS.md`.

**État actuel : v0.4.0 validée** (28/10/2026 : sortie vidéo de la vue de
dessus en Spout et NDI, après la v0.2.0 : zones rectangle / ellipse / forme
libre, métriques, anti-clignotement, slots de joueurs « SwapSlots »,
Tracking Monitor complet ; la v0.3, éditeur 2D, est en grande partie faite
dans le Tracking Monitor).
**Prochaine étape : v0.5**, entrée PSN, multi-sources,
transformation par source.

## 2. Comment travailler avec Vincent

- **Répondre en français.** Le code, les noms de fichiers, les commits et les
  commentaires dans le code sont en anglais. **Tout ce qui est visible dans
  Chataigne** (noms de modules, paramètres, valeurs, panneaux, menus,
  messages de log) est aussi **en anglais** (règle de Vincent, 26/10/2026).
- Vincent n'est pas développeur C++. Il lit le code et comprend la logique
  (il pratique Python). Expliquer ce qu'on fait et pourquoi, simplement.
- **Étape par étape.** Une étape à la fois, attendre sa validation avant la
  suivante. Ne pas empiler plusieurs étapes dans un même message.
- **Vérifier avant d'affirmer.** Lire le code existant, chercher la
  documentation en ligne si besoin. Ne pas supposer qu'une chose marche :
  le démontrer (compilation, test).
- **Petites briques testables.** Chaque brique doit compiler et pouvoir être
  testée par Vincent dans Chataigne avec Augmenta Fusion.
- Rôles : Claude écrit le code, compile, corrige. Vincent définit le
  comportement attendu, teste avec du vrai tracking, valide.
- **Traçabilité : `TODO.md`** (racine du dépôt ; versions terminées dans `docs/drawzone/TODO-archive.md`). Le lire en début de séance.
  Le mettre à jour après chaque étape : réussites (✅), échecs (❌) avec leur
  cause et leur correction, étape en cours (⏭️), à faire (⬜), idées (💡).
  Les échecs sont notés aussi, pas seulement les succès.
- Pas de commit ni de push sans l'accord de Vincent.

## 3. Arborescence locale (Windows uniquement pour l'instant)

```
D:\0-DRAWZONE\
├── JUCE\        JUCE modifié de Ben (branche develop-local, JUCE 7.0.12). Hors dépôt.
└── DrawZone\    CE dépôt (fork : github.com/vincethpiafff/DrawZone)
```

Ben interdit de placer JUCE à l'intérieur du dépôt (clone, copie, sous-module,
lien symbolique). On respecte cette règle.

## 4. Compiler et lancer

Procédure complète d'installation et dépannage : `docs/drawzone/BUILDING.md`.

Au quotidien, depuis la racine du dépôt :

```bat
build_dev.bat
start "" "Binaries\App\Chataigne.exe"
```

- `build_dev.bat` = compilation Release rapide (sans LTCG), incrémentale.
  L'exe sort dans `Binaries\App\`.
- MSBuild n'est pas dans le PATH d'un terminal normal. Son chemin :
  `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe`
  (`build_dev.bat` le trouve tout seul).
- Il n'y a que les **Build Tools** VS 2022, pas Visual Studio complet :
  ne jamais proposer « Open in IDE » ou une action qui suppose l'IDE.
- Après un nouveau clone : lancer `Tools\drawzone\setup_windows.bat`
  (WebView2 + DLL de dépendances).
- Au lancement, des messages « DNSServiceDiscovery error -65563 » apparaissent
  dans la console : service Bonjour absent, sans conséquence.
- `run_debug.cmd` / `Tools/vscode/debug.ps1` de Ben **ne marchent pas chez nous**
  (chemin JUCE de Ben écrit en dur). Ne pas les utiliser.

### Ajouter des fichiers source : passer par le Projucer

Chataigne est généré à partir de `Chataigne.jucer`. Pour ajouter ou retirer un
fichier source, il faut l'ajouter au `.jucer` puis **enregistrer dans le
Projucer**, qui régénère tous les projets (Windows, Mac, Linux…).

- Projucer : `D:\0-DRAWZONE\JUCE\extras\Projucer\Builds\VisualStudio2022\x64\Release\App\Projucer.exe`
  (compilé en **mode GPL**, voir BUILDING.md ; sinon l'enregistrement est bloqué).
- Ligne de commande possible : `Projucer.exe --resave Chataigne.jucer`.
- Après chaque enregistrement, contrôler `git status` : seuls les fichiers
  attendus doivent changer.

### Le « unity build » de Chataigne

Les `.cpp` de Chataigne ne sont presque jamais compilés directement
(`compile="0"` dans le `.jucer`). Ils sont inclus dans quelques gros fichiers :

- en-têtes déclarés dans `Source/Module/ModuleIncludes.h` ;
- `.cpp` inclus dans `Source/Module/ModuleIncludes.cpp`, `ModuleIncludes2.cpp`,
  `ModuleIncludes3.cpp` (exemple : PosiStageNet, ligne
  `#include "modules/posistagenet/PosiStageNetModule.cpp"` dans `ModuleIncludes3.cpp`) ;
- enregistrement d'un module dans `Source/Module/ModuleFactory.cpp`
  (`defs.add(new ModuleDefinition("Protocol", "PosiStageNet", &PosiStageNetModule::create));`).

Le module Spatial suivra ce même schéma (à confirmer en début de v0.1) :
code dans `Source/Module/modules/spatial/`.

## 5. Règles Git

- Branches : `master` = Chataigne d'origine de Ben (ne pas y toucher),
  `develop` = intégration DrawZone, `feature/<nom>` = une brique en cours.
  Une branche `main` (stable DrawZone) sera créée à la première version stable.
- Remotes : `origin` = notre fork ; `upstream` = Chataigne de Ben en
  **lecture seule** (son URL de push vaut `NE_PAS_PUSHER`). Ne jamais changer ça.
  Même blocage sur les 6 sous-modules (`Modules/juce_*`) et sur
  `D:\0-DRAWZONE\JUCE`, dont `origin` est un dépôt de Ben. Ce réglage est local :
  à refaire après un nouveau clone
  (`git submodule foreach "git remote set-url --push origin NE_PAS_PUSHER"`).
- **Ne pas déranger Ben** (le fork est public et relié au sien) :
  - ne jamais ouvrir de Pull Request vers `benkuper/Chataigne`. Sur un fork,
    GitHub propose ce dépôt par défaut : pour une PR interne, vérifier que
    « base repository » vaut `vincethpiafff/DrawZone`. Ne pas utiliser
    « Contribute → Open pull request » ;
  - ne jamais écrire `@benkuper` ni `benkuper/<dépôt>#<n°>` dans un commit,
    un ticket ou une PR (cela le notifie ou ajoute un lien chez lui) ;
  - les GitHub Actions restent désactivées sur le fork (Settings → Actions →
    General → « Disable actions ») ;
  - « Sync fork » sur `master` est sans risque : il récupère, il n'envoie rien ;
  - un build DrawZone n'envoie rien aux serveurs de Ben : bloc `// DrawZone:`
    dans `Source/Main.cpp` (mises à jour, rapports de plantage, statistiques
    Matomo, conversion en ligne des fichiers). À garder lors des fusions avec
    upstream, et à étendre si Ben ajoute un nouvel envoi.
- **Fins de ligne** : les fichiers de Ben mélangent parfois CRLF et LF dans un
  même fichier (ex. `Source/Main.cpp`), et le `.jucer` est en CRLF. Après
  chaque modification d'un fichier de Ben, `git diff --stat` doit montrer
  uniquement nos lignes. Ne pas utiliser `sed` de Git Bash (il convertit) ;
  l'outil d'édition peut aussi uniformiser : vérifier, et sinon insérer
  à l'octet près avec `perl -0777 -pi`.
- Commits au format Conventional Commits, en anglais :
  `feat(spatial): ...`, `fix(...)`, `docs(...)`, `chore(build): ...`.
- Versions SemVer propres à DrawZone avec tags annotés (`v0.1.0`…), et
  mise à jour de `docs/drawzone/CHANGELOG.md` à chaque version.
- **`core.autocrlf=false`** dans le dépôt et les 6 sous-modules. Indispensable :
  avec la conversion automatique de Git for Windows, le Projucer régénère
  `BinaryData` et `JuceLibraryCode` différemment (≈1000 lignes parasites).
- Ne jamais committer : `Binaries/`, `dependencies_zips/`,
  `Builds/VisualStudio2022/packages/`, les `JucePrecompiledHeader_*`
  (tout est déjà ignoré par le `.gitignore` de Ben).
- Détails : `docs/drawzone/CONTRIBUTING.md`.

## 6. Ne pas diverger inutilement du code de Ben

On veut pouvoir récupérer les mises à jour de Chataigne (`git fetch upstream`)
et, un jour, proposer notre travail à Ben.

- Mettre notre code dans **nos propres fichiers et dossiers**.
- Dans le code de Ben, ne faire que des modifications minimales et
  nécessaires (enregistrer un module, ajouter un include), signalées par un
  commentaire `// DrawZone: ...`.
- Ne pas corriger ni reformater le code de Ben « au passage ».
- Fichiers propres à DrawZone : `CLAUDE.md`, `.github/README.md`,
  `docs/drawzone/`, `Tools/drawzone/`. Le `README.md` racine reste celui de Ben.
- Seule divergence connue à ce jour : `Builds/VisualStudio2022/Chataigne_App.vcxproj`
  pointe vers notre JUCE (`D:\0-DRAWZONE\JUCE`). En cas de conflit lors d'une
  fusion avec upstream : prendre la version de Ben puis réenregistrer dans le Projucer.

## 7. Fichiers de Ben à ne pas suivre

- `AGENTS.md` : instructions de Ben pour **ses** assistants (son chemin JUCE
  `D:\Projects\Dev\JUCE`, sa procédure de publication de Chataigne, SFTP de son
  site). **Ne s'applique pas à DrawZone.** Ne jamais exécuter sa procédure de release.
- `MCP PLAN.md`, `Tools/mcp/` : travail en cours de Ben sur un serveur MCP. Pas à nous.
- `.github/workflows/build.yml` : CI de Ben, se déclenche à chaque push et
  dépend de ses secrets. Elle doit rester **désactivée** sur notre fork
  (onglet Actions de GitHub).

## 8. Tests avec Augmenta Fusion

- Module OSC dans Chataigne : port d'entrée 12000 par défaut.
- Augmenta Fusion : sortie OSC vers `127.0.0.1:12000`.
- Messages observés et points à vérifier : `docs/drawzone/notes/augmenta-protocol.md`.
- Pour PSN, il faudra écrire un petit émetteur de test en Python (v0.5).
