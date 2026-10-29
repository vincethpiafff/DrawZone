# Compiler DrawZone sous Windows

Procédure validée le 23 octobre 2026 sur Windows 11 x64.
Chaque étape indique le résultat attendu. La section « Dépannage » en fin de
document explique les pièges rencontrés et pourquoi on fait les choses ainsi.

## 0. Prérequis

| Outil | Remarque |
|---|---|
| Git for Windows | Avec un compte GitHub. |
| Build Tools for Visual Studio 2022 | Visual Studio complet n'est pas nécessaire. |
| Python 3 | Pour les futurs outils de test. |
| VS Code + Claude Code | Éditeur de travail. |

### Installer les Build Tools VS 2022

Le plus simple, dans un terminal :

```bat
winget install Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

Autre possibilité : télécharger « Build Tools for Visual Studio 2022 » sur
https://visualstudio.microsoft.com/vs/older-downloads/ et cocher la charge de
travail **« Développement Desktop en C++ »**. Doivent être cochés :
**MSVC v143** et **Windows 11 SDK** (10.0.26100 ou plus récent).
Inutile : ATL, MFC, C++/CLI, anciennes versions MSVC.

MSBuild est alors ici (il n'est pas dans le PATH d'un terminal normal) :

```
C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe
```

## 1. Dossier de travail et JUCE

```bat
mkdir D:\0-DRAWZONE
cd /d D:\0-DRAWZONE
git config --global core.longpaths true
git clone --branch develop-local --depth 1 https://github.com/benkuper/JUCE JUCE
```

JUCE reste **à côté** du dépôt, jamais dedans (règle de Ben).

## 2. Cloner DrawZone sans conversion des fins de ligne

```bat
cd /d D:\0-DRAWZONE
git -c core.autocrlf=false clone --recursive https://github.com/vincethpiafff/DrawZone DrawZone
cd DrawZone
git config core.autocrlf false
git submodule foreach "git config core.autocrlf false"
git checkout develop
git remote add upstream https://github.com/benkuper/Chataigne
git remote set-url --push upstream NE_PAS_PUSHER
```

Attendu : `git status` affiche « working tree clean » et `git remote -v`
montre `upstream ... NE_PAS_PUSHER (push)`.

## 3. Compiler le Projucer en mode GPL

Le Projucer transforme `Chataigne.jucer` en projet Visual Studio.

```bat
cd /d D:\0-DRAWZONE\JUCE\extras\Projucer\Builds\VisualStudio2022
set CL=/DJUCER_ENABLE_GPL_MODE#1
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" Projucer.sln /p:Configuration=Release /p:Platform=x64 /m /t:Rebuild
set CL=
```

- Le `#` remplace `=` (syntaxe Microsoft de la variable `CL`).
- `set CL=` à la fin évite que l'option se glisse dans d'autres compilations.
- Attendu : `Build succeeded`, `0 Error(s)` (≈40 s).
- Résultat : `x64\Release\App\Projucer.exe`. Au lancement, le logo **GPL**
  apparaît en haut à droite.

## 4. Régler les chemins globaux et régénérer le projet

1. Lancer `Projucer.exe`.
2. **File → Global Paths…**
   - Path to JUCE : `D:\0-DRAWZONE\JUCE`
   - JUCE Modules : `D:\0-DRAWZONE\JUCE\modules`
   - Le reste (VST, AAX, ARA, Android, User Modules) ne sert pas.
   - Ne cliquer sur aucun bouton « Re-scan », fermer la fenêtre.
3. **File → Open…** → `D:\0-DRAWZONE\DrawZone\Chataigne.jucer`
4. **Ctrl+S** (ne pas utiliser « Save and Open in IDE »).

Contrôle :

```bat
cd /d D:\0-DRAWZONE\DrawZone
git status --short
```

Sur un dépôt déjà configuré (cas normal, le commit de v0.0.0 contient le
projet régénéré) : **aucune ligne**. Sur un clone de Chataigne d'origine :
une seule ligne, ` M Builds/VisualStudio2022/Chataigne_App.vcxproj`.

## 5. Dépendances (WebView2 et DLL)

```bat
Tools\drawzone\setup_windows.bat
```

Ce script (sans effet s'il a déjà été lancé) :

1. copie le paquet WebView2 de `Builds\VisualStudio2022_CI\packages` vers
   `Builds\VisualStudio2022\packages` ;
2. télécharge le pack de DLL de Ben dans `dependencies_zips\` s'il est absent
   (`Chataigne-win-x64-release-dependencies.zip`, ≈28 Mo) ;
3. l'extrait dans `Binaries\App\`.

Tous ces dossiers sont ignorés par Git.

Équivalent manuel :

```bat
xcopy /E /I /Y Builds\VisualStudio2022_CI\packages Builds\VisualStudio2022\packages
mkdir dependencies_zips
curl -L -o dependencies_zips\Chataigne-win-x64-release-dependencies.zip https://benjamin.kuperberg.fr/chataigne/user/data/Chataigne-win-x64-release-dependencies.zip
mkdir Binaries\App
tar -xf dependencies_zips\Chataigne-win-x64-release-dependencies.zip -C Binaries\App
```

## 6. Compiler et lancer

```bat
build_dev.bat
start "" "Binaries\App\Chataigne.exe"
```

- Attendu : `Build succeeded`, `0 Error(s)`. Deux avertissements du code de
  Ben sont normaux (`StringComparators.cpp` C4101, `ChataigneGPIO.c` C4206).
- Première compilation : quelques minutes ; ensuite, incrémentale.
- `build.bat` fait la même chose avec l'optimisation finale (LTCG, 10 à 20 min
  de plus) : à garder pour les versions publiées.
- Depuis le terminal de Claude Code (Git Bash), `build_dev.bat` échoue
  (« 'C:\Program' is not recognized », guillemets de `cmd`). Appeler
  MSBuild directement, avec les mêmes options :

  ```sh
  "/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/MSBuild/Current/Bin/MSBuild.exe" \
    Builds/VisualStudio2022/Chataigne.sln -p:Configuration=Release -p:Platform=x64 \
    -p:PreferredToolArchitecture=x64 -p:WholeProgramOptimization=false \
    -p:LinkTimeCodeGeneration=Default -m -v:minimal
  ```

  (`build_dev.bat` copie aussi les DLL mpv à la fin : à relancer depuis un
  terminal Windows si elles manquent.)

## 7. Test de validation

1. Dans Chataigne : Modules → **+** → Protocol → **OSC**. Port d'entrée
   (Local Port) : 12000. Cocher **Log Incoming**.
2. Dans Augmenta Fusion : sortie OSC vers `127.0.0.1`, port 12000.
3. Le Logger de Chataigne affiche `/scene`, `/fusion`, `/object/update`,
   `/object/leave`.

## Dépannage

| Symptôme | Cause | Solution |
|---|---|---|
| `aka.ms` injoignable dans le navigateur | DNS ou bloqueur | Installer les Build Tools via `winget` (étape 0). |
| `'msbuild' is not recognized` | MSBuild absent du PATH | Utiliser le chemin complet (étape 0). |
| `C1083: Cannot open source file '.\JucePrecompiledHeader_Release.cpp'` | Projet pas régénéré sur cette machine : ces fichiers sont créés par le Projucer et ignorés par Git | Étape 4. |
| Le projet cherche `D:\Projects\Dev\JUCE` | Chemin global de Ben dans le projet d'origine | Étape 4 (chemins globaux). |
| Projucer : « Incompatible License and Splash Screen Setting… Save and export is disabled » | Projucer standard = licence commerciale ; Chataigne désactive le splash screen, ce qui n'est permis qu'en GPL | Étape 3 (mode GPL). Ne pas cliquer sur « Sign in » ni « Enable splash screen ». |
| Après Ctrl+S, `BinaryData.cpp/.h` et tout `JuceLibraryCode/` apparaissent modifiés | Git for Windows convertit LF → CRLF au clone ; le Projucer embarque ces octets dans BinaryData | Recloner avec `core.autocrlf=false` (étape 2). |
| Avertissements « LF will be replaced by CRLF » en masse | Même cause | Idem. |
| Git affiche « Log file is already in use (press RETURN) » | Afficheur page par page de Git | Entrée puis `q` ; utiliser `git --no-pager ...`. |
| `C1083: Cannot open include file: 'WebView2.h'` | Paquet NuGet WebView2 non restauré (pas d'IDE) | Étape 5. |
| « JoyShockLibrary.dll / Kinect20.dll / Servus.dll / SDL2.dll was not found » | DLL du pack non extraites | Étape 5. |
| Console : `DNSServiceDiscovery error: -65563` | Service Bonjour non installé | Sans conséquence. |

## Mettre à jour JUCE ou le Projucer

```bat
cd /d D:\0-DRAWZONE\JUCE
git pull
```

Puis recompiler le Projucer **en mode GPL** (étape 3), réenregistrer
`Chataigne.jucer` (étape 4) et vérifier `git status`.
