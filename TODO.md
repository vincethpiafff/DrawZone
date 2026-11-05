# TODO — DrawZone

Lire en début de séance avec `CLAUDE.md`. Fini : `docs/drawzone/TODO-archive.md`.
Légende : ✅ fait · ❌ échec / écarté (+ cause) · ⏭️ en cours · ⬜ à faire · ⏸️ reporté · 💬 Vincent · 💡 idée

**Écrire court** : une ligne par étape (date, quoi, résultat, chiffres du test).
Pas de récit, pas de répétition. Version terminée → archive, au mot près.

## ⏭️ En cours : v0.6, viewport 3D

But (💬 31/10) : plusieurs scènes reçues de Fusion, placées en 3D par DrawZone ;
d'abord voir la scène en 3D, bascule 2D / 3D dans le Tracking Monitor, formes simples.

- ✅ 31/10 étude, sans code. Une seule source Augmenta reçoit l'OSC (un port par
  module) ; `/scene` v2 sans nom ; piste non vérifiée : un port par scène Fusion.
- Briques proposées : 1 vue 3D lecture seule (dessin JUCE du Monitor, sans OpenGL) ·
  2 source Augmenta avec son port · 3 placement 3D par scène · 4 formes simples ·
  5 test plusieurs scènes Fusion.
- 💬 05/11 Vincent : test à 2 systèmes de tracking → bonus (cas réel) ; tests de dév avec
  Fusion ; module TrackSimu en v0.9, après le graphe nodal (D16).
- 💬 05/11 Vincent : briques 1 à 4 validées, zones **A** (chaque zone appartient à une scène).
- ⬜ Vincent, avant la brique 2 : 2e scène Fusion, sortie OSC sur 12001 → capture.
- ⏭️ 05/11 brique 1, code (branche `feature/viewport-3d`) : `ui/Viewport3DRenderer.h/.cpp` (sol + grille 1 m, axes,
  zones au sol, personnes = ombre + corps + tête à leur hauteur, 1,7 m par défaut) ; bascule « 2D | 3D »
  dans l'en-tête du Monitor ; 3D en lecture seule : glisser = orbite, clic droit = déplacer, molette = zoom,
  double clic = recadrer ; dessiner une zone repasse en 2D ; `.jucer` + Projucer (+38 lignes).
- ✅ 05/11 recompilation complète 0 erreur ; `run_checks.ps1` : PSN 30 / 30 ×2, Source IDs 6 / 6, Id Offset 4 / 4,
  Augmenta 36 / 36 ; bascule visible (capture) ; logs propres. ❌ clic auto sur « 3D » : Windows refuse
  le premier plan, clic parti ailleurs → pas d'automatisation de souris. ✅ Vincent : « ok », brique 1 validée.
- ✅ 05/11 capture Fusion : 1 port par scène confirmé (12001 : `/scene` 8 x 3, `/fusion`, `/object/update` + `/extra`,
  1 message par paquet, sans bundle) ; 213 msg/s expliqué (2 personnes x 2 messages x ≈ 48 Hz + `/scene` et `/fusion` à 10 Hz).
- ⏭️ 05/11 brique 2, code : `Port` par source Augmenta (port du module → entrée du module, autre port → `OSCReceiver`
  à elle) ; nouvelle source : port + 1, Id Offset + 100 ; recompilation complète 0 erreur.
  ❌ tests 17/30, 3/6, 3/4, 14/36 : Port vaut 12000 par défaut, les copies de test mettent le module sur 12999,
  donc la source ouvre 12000 (Fusion) et n'entend plus le simulateur. Même risque pour une ancienne session
  dont l'entrée OSC n'est pas sur 12000. **Arrêt, décision avec Vincent.**
- ✅ 05/11 💬 Vincent : option A. Case **Use Module Input** (cochée par défaut et pour les anciennes sessions ; Port grisé) ;
  nouvelle source : décochée, port 12001+, Id Offset +100. Test ajouté (`check_sim.ps1`, 2e scène sur 12998, Offset 8 m).
  Recompilation complète 0 erreur ; PSN 30 / 30 ×2, Source IDs 6 / 6, Id Offset 4 / 4, Augmenta **41 / 41**.
  Test : les logs ignorent `CommunityModuleManager` (liste en ligne, « Failed to connect » le 05/11). 
- ❌ 05/11 💬 Vincent : une source ajoutée arrive avec Use Module Input coché. Cause probable : le menu passe le JSON
  de la source à `addItemInternal` quand plusieurs conteneurs sont sélectionnés (`addItemForSelected`), le test
  `data.isVoid()` l'écartait. Correction : « nouvelle » = pas en chargement de fichier. Compilé 0 erreur ;
  PSN 30 / 30 ×2, Source IDs 6 / 6, Id Offset 4 / 4, Augmenta 41 / 41 (le menu n'est pas testable en simulation).
  ✅ Vincent : « OK », brique 2 validée.
- 💬 05/11 Vincent, pour la brique 3 : en 2D, un sélecteur de la scène affichée (SOL ou MUR) ; en 3D, une liste
  des scènes avec Position et Rotation X / Y / Z modifiables.
- 💬 05/11 Vincent : option A, bloc **Scenes** (une source et une zone appartiennent à une scène).
- ⏭️ 05/11 brique 3, code : `SpatialScene.h/.cpp` (Size Mode Auto / Manual, Size, Position et Rotation X / Y / Z,
  Status) ; paramètre Scene (`TargetParameter`) dans chaque source et chaque zone, vide = 1re scène ; Use As Scene
  par scène ; zones calculées avec les personnes de leur scène ; ancien bloc Scene caché, repris dans la 1re scène ;
  Monitor : pastille de scène dans l'en-tête (menu), 2D filtrée sur la scène, nouvelle zone dans la scène affichée ;
  3D : chaque scène à sa place, liste des scènes avec Position / Rotation modifiables ; sortie vidéo = 1re scène.
- ❌ 05/11 Chataigne plante au démarrage (`SpatialSceneManager::addItemInternal`, journal du CrashHandler dans
  `%APPDATA%Chataigneog_*.txt`) : `itemColor` nul, `setHasCustomColor(true)` oublié → corrigé.
- ✅ 05/11 recompilation 0 erreur ; PSN 30 / 30 ×2, Source IDs 6 / 6, Id Offset 4 / 4, Augmenta 41 / 41. Tests : libellé
  « Waiting for the scene size » accepté, logs `CommunityModuleManager` et `HelpBox` ignorés (internet, pas DrawZone).
  ⬜ test de Vincent. Limites : slots tenus et sortie PSN sans notion de scène.
- ✅ 05/11 reprise : 1er passage PSN 28 / 30 ×2, Augmenta 40 : session enregistrée avec Spout et NDI décochés, donc
  Resolution vide (calculée seulement si sortie active). `make_psn_session.ps1` coche Spout dans chaque copie →
  PSN 30 / 30 ×2, Source IDs 6 / 6, Id Offset 4 / 4, Augmenta 41 / 41.
- ✅ 05/11 💬 Vincent : brique 3 validée → `50c515de`.
- ✅ 05/11 💬 Vincent : en 3D, personne = cercle + croix sur le centroïde (plus de corps ni de tête) ; compilé 0 erreur ;
  ✅ Vincent : « ok ». Tests en simulation désormais seulement si logique ou données changent (`CLAUDE.md`).
- ⏸️ 05/11 💬 Vincent : brique 4 (formes / zones en 3D) → bonus, si un jour un tracking 3D (ZacTrack, lidar 3D).
- 💬 05/11 Vincent : scène → **surface** (un plan du monde réel : sol, mur) ; slots par surface ; PSN = nom du slot / tracker
  + surface puis valeurs, format « JOUEUR 1 (SOL) », n° = centaine par surface (SOL 1.., MUR 101..) ; 1 sortie vidéo
  par surface, flux nommé comme la surface.
- ✅ 06/11 S1-S4 : Scene → Surface partout (code, Chataigne, tests, ROADMAP, D17) ; Player Slots et Video Output dans chaque
  surface (Values > Slots > <surface>, flux Spout / NDI = nom de la surface, champ Stream Name retiré) ; valeur Surface par
  personne ; PSN « JOUEUR 1 (SOL) », SOL 1.., MUR 101.. ; anciennes sessions reprises (blocs Scene, Scenes, Player Slots,
  Video Output → 1re surface). Bug corrigé : options Display de la sortie vidéo non enregistrées.
  Recompilation complète 0 erreur ; PSN 30 / 30 ×2, Source IDs 6 / 6, Id Offset 4 / 4, Augmenta 46 / 46 (+5 : MUR, ses slots,
  sa sortie vidéo) ; test ponctuel PSN : SOL → 1 « JOUEUR 1 (SOL) », MUR → 101 « Slot 1 (MUR) ». Copies de test : surfaces
  SOL + MUR fixes. ⚠️ adresses changées : Values > Slots > <surface> > <slot> (State Machine à repointer). ⬜ test de Vincent.
- ⏭️ 06/11 💬 Vincent : slots on/off par surface (off → PSN avec l’id du tracking, conflit d’id signalé dans le Status) ;
  vue 3D : liste des surfaces repliable (en-tête « Surfaces », chaque surface dépliable). Test `check_psn_surfaces.ps1`
  (2 surfaces, MUR slots on → 60101, off → 1005).
- ⬜ brique 5 : test à plusieurs scènes Fusion (SOL 12000 + MUR 12001).

## 🔧 Étape d'optimisation, revue du 06/11/2026

Relu sans rien modifier : `spatial/` (≈ 7 300 lignes), `Tools/drawzone/`, tous les `.md`. Suite des numéros de la revue
du 28/10 (archive). Recommandation : B4 à décider d'abord, puis B5, B6, le nettoyage 26 + 29 à 34, les docs D1 à D6 à
la clôture de la v0.6, 28 ensuite ; 27 (gros) plus tard.

#### 🐞 Bugs

| # | État | Où | Constat | Proposition | Effort |
|---|---|---|---|---|---|
| B4 | ✅ 06/11 | `SpatialPSNOutput` | Numéros PSN qui se chevauchent : slots 1.., 101.. ; ids du tracking d'une surface sans slots : Augmenta 101.. (Id Offset par défaut), PSN 1.. → la personne n'est pas envoyée (signalé dans le Status) | 💬 Vincent : plage haute. Slots 60001.. (60101.. pour la 2e surface), tests mis à jour | Petit |
| B5 | ✅ 06/11 | `SpatialVideoOutput::timerCallback` | Spout / NDI continuent quand le module Cockpit est désactivé (la sortie PSN, elle, s'arrête) | Fait : rien n'est envoyé, Resolution « Nothing sent : the module is disabled » | Petit |
| B6 | ✅ 06/11 | Surfaces | Deux surfaces de même nom : deux flux Spout / NDI identiques, adresses en double dans Values > Slots | Fait : avertissement dans le Status de la surface | Petit |

#### 🟠 Nettoyage et doublons

| # | État | Type | Où | Constat | Proposition | Effort |
|---|---|---|---|---|---|---|
| 26 | ✅ 06/11 | Reste de la 1re version | `TrackingData`, `SpatialModule::updateZones` | Taille « de scène » unique gardée dans `TrackingData` (`getSurfaceSize` jamais appelé) ; `zonesSurfaceSize` recopiée sans raison | Retirer de `TrackingData` ; une zone ajoutée par l'Inspector se centre sur sa surface | Petit |
| 27 | ⬜ plus tard | Taille | `TrackingMonitorPanel.cpp` (2 300 lignes) | Un seul fichier pour la vue 2D, l'édition, la colonne, les slots, la 3D | Le couper en 4 fichiers (édition des zones, colonne, barre des slots, panneau 3D) | Gros |
| 28 | ✅ 06/11 | Doublon | `Tools/drawzone/sim/*.ps1` | `Check`, `Near`, `Sim`, client MCP recopiés dans 6 scripts ; fermer / ouvrir Chataigne dans `run_checks` et `build_and_check` | Un `common.ps1` partagé | Moyen |
| 29 | ✅ 06/11 | Code mort | `SpatialModule::onControllableFeedbackUpdateInternal` | Ne fait qu'appeler la classe mère | Supprimer | Petit |
| 30 | ✅ 06/11 | Lisibilité | `SpatialSourceManager::addItemInternal` | `if (maxOffset >= 0)` écrit deux fois de suite | Un seul bloc | Petit |
| 31 | ✅ 06/11 | Rangement | `Viewport3DRenderer.cpp` | `Viewport3DDetail` coupé en deux par `Camera::pan` | Regrouper | Petit |
| 32 | ✅ 06/11 | Commentaire | `SpatialZone.cpp` | Commentaire des bornes (« finite bounds… ») posé au-dessus de `contains()` | Le remettre près de `setBounds` | Petit |
| 33 | ❌ 06/11 | Bruit | `build_dev.bat` | Chaque build affiche « 'C:\Program' is not recognized » (guillemets autour de vswhere) | Écartée : `build_dev.bat` est un fichier de Ben (règle : on n'y touche pas) et le message est sans effet, MSBuild est trouvé ensuite | Petit |
| 34 | ✅ 06/11 | Robustesse | `AugmentaSource` (port propre) | Le thread réseau lit `niceName` sans verrou : copie de texte pendant un renommage | Garder le nom dans une copie protégée, mise à jour à 4 Hz | Petit |

#### 📄 Fichiers .md (moins de lignes à relire, une seule source par sujet)

| # | État | Fichier | Constat | Proposition | Effort |
|---|---|---|---|---|---|
| D1 | ✅ 06/11 | `.github/README.md` | Statut « v0.0.0 » faux ; tableau de version recopié de la ROADMAP ; lien vers le journal figé | Statut à jour, renvoi vers la ROADMAP au lieu du tableau | Petit |
| D2 | ✅ 06/11 | `ROADMAP.md` | « Détail des prochaines versions » = v0.1 à v0.4, finies (≈ 60 lignes, déjà dans le CHANGELOG) | Supprimer ; garder tableau, bonus, points durs | Petit |
| D3 | ✅ 06/11 | `CONTRIBUTING.md` / `CLAUDE.md` | Branches, fichiers à ne jamais commiter, fins de ligne, code de Ben écrits deux fois ; « spatial/ (prévu) » périmé | Une source : CONTRIBUTING pour les règles, CLAUDE.md garde le propre à Claude + renvoi | Moyen |
| D4 | ✅ 06/11 | `JOURNAL.md` | Figé depuis le 29/10, encore lié depuis le README | Le verser dans `TODO-archive.md`, supprimer le fichier | Petit |
| D5 | ⏭️ question retirée ; v0.6 à archiver à la clôture | `TODO.md` | Partie v0.6 ≈ 70 lignes ; question « place du bloc Scene » périmée | À la clôture : v0.6 vers l'archive, question retirée | Petit |
| D6 | ⏭️ CHANGELOG « Non publié » rempli ; CLAUDE.md et tag à la clôture | `CHANGELOG.md`, `CLAUDE.md` | « Non publié » vide malgré la v0.6 ; CLAUDE.md dit « État : v0.5.0 » | Remplir à la clôture (tag v0.6.0) | Petit |

- ✅ 06/11 revue faite (B4-B6, 26, 28-34, D1-D4) : recompilation complète 0 erreur (C4101 / C4206 de Ben) ; PSN 30 / 30 ×2,
  Source IDs 6 / 6, Id Offset 4 / 4, 2 surfaces 5 / 5 ×2 (slots MUR on → 60101, off → 1005), Augmenta 46 / 46.
  ❌ puis ✅ `make_psn_session.ps1` : variable `$surfaces` = paramètre `-Surfaces` (PowerShell ignore la casse) → `$surfacesJson`.
  ⬜ test de Vincent, puis commit sur son accord.
- ✅ 06/11 💬 Vincent : on / off des slots dans le viewport → case « Slots » dans la barre des slots (surface affichée) et sur
  chaque ligne de la liste des surfaces en 3D. Compilé 0 erreur ; affichage seul, pas de test en simulation. ⬜ œil de Vincent.

## ⬜ Questions ouvertes

- Choix de la carte réseau pour l'entrée PSN (la sortie l'a déjà).
- Kratos réel : signe de l'orientation non vérifié ; 2 émetteurs sur un même groupe
  PSN se mélangent (limite connue).

## ⬜ Plus tard

- **Script SMODE** `Tools/drawzone/smode/drawzone_osc_bank.py` (✅ écrit et testé hors
  SMODE, 28/10) : ⬜ test de Vincent dans SMODE après la v0.7 (sorties OSC du graphe).
  Non confirmé : types `Parameter(Real/Integer/String)`, `p.value.set` après l'append.
- Optimisation reportée : 17 (rendu vidéo sur un thread à part, si besoin mesuré),
  18 (copie unique de `TopViewModel`, à mesurer), 23 (Monitor limité au 1er Cockpit).
- Inspector : barre d'icônes verticale (style C), après le style D.
- Réglages d'interface du Tracking Monitor : quand toutes ses briques seront finies.

## 💡 Idées

- 💡 26/10 Tracking Monitor plus « user friendly » (Vincent) : choix du module,
  vue et tableau redimensionnables, zoom, flèche de vitesse, colonnes au choix,
  survol personne ↔ ligne du tableau.
