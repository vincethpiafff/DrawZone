# Conventions de travail

## Branches

| Branche | Rôle |
|---|---|
| `master` | Chataigne d'origine de Ben. On n'y touche pas. |
| `develop` | Intégration DrawZone. Base de tout travail. |
| `feature/<nom>` | Une brique en cours (ex. `feature/spatial-module`). |
| `main` | Versions stables de DrawZone (créée à la première version stable). |

Cycle d'une brique :

```bat
git checkout develop
git pull
git checkout -b feature/<nom>
rem ... travail, compilations, tests ...
git add <fichiers>
git commit -m "feat(spatial): ..."
git push -u origin feature/<nom>
rem une fois testé et validé par Vincent :
git checkout develop
git merge --no-ff feature/<nom>
git push origin develop
```

## Messages de commit

Format [Conventional Commits](https://www.conventionalcommits.org/), en anglais :

```
<type>(<portée>): <résumé à l'impératif>
```

- Types : `feat`, `fix`, `docs`, `refactor`, `test`, `chore`, `build`.
- Portées usuelles : `spatial`, `zones`, `editor2d`, `viewport3d`, `graph`,
  `build`, `docs`.
- Exemple : `feat(spatial): add rectangle zone presence boolean`.

## Versions

- SemVer propre à DrawZone, indépendante de la version de Chataigne.
- À chaque version : mettre à jour `docs/drawzone/CHANGELOG.md`, puis
  ```bat
  git tag -a v0.1.0 -m "DrawZone v0.1.0 - ..."
  git push origin v0.1.0
  ```

## Récupérer les mises à jour de Chataigne

```bat
git fetch upstream
git log --oneline develop..upstream/master
rem si on décide d'intégrer :
git checkout develop
git merge upstream/master
git submodule update --init --recursive
```

Puis réenregistrer `Chataigne.jucer` dans le Projucer, recompiler et tester.
Conflit sur `Builds/VisualStudio2022/Chataigne_App.vcxproj` : prendre la
version de Ben puis réenregistrer dans le Projucer.

`upstream` est en lecture seule (URL de push `NE_PAS_PUSHER`) : on ne peut
pas envoyer par erreur chez Ben.

## Règles de code

- Notre code dans nos fichiers : `Source/Module/modules/spatial/` (prévu),
  `Tools/drawzone/`, `docs/drawzone/`.
- Dans les fichiers de Ben : modifications minimales, commentées
  `// DrawZone: <raison>`.
- Suivre le style de Chataigne (classes organicUI : `ControllableContainer`,
  `Parameter`, `Trigger`…) pour que l'interface reste cohérente.
- Ajouter un fichier source = l'ajouter au `.jucer` + enregistrer dans le Projucer.

## Fins de ligne

`core.autocrlf=false` partout (voir DECISIONS.md, D9). Le `.gitattributes` de
Ben force LF pour `*.md`, `*.yml`, `.github/**`, etc. Les scripts `.bat` de
DrawZone sont en CRLF.

## Ce qui ne doit jamais être commité

`Binaries/`, `dependencies_zips/`, `Builds/VisualStudio2022/packages/`,
`Builds/VisualStudio2022/x64/`, `JucePrecompiledHeader_*`, `.vscode/`.
Tout est déjà dans le `.gitignore` de Ben ; vérifier `git status` avant chaque commit.
