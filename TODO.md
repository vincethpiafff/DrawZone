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
- ⏸️ 05/11/2026 fin de séance. Reprise : test de Vincent sur la brique 3 (5 points donnés le 05/11), puis commit sur son
  accord (brique 3 non commitée sur `feature/viewport-3d`) ; ensuite brique 4, formes simples.

## ⬜ Questions ouvertes

- Choix de la carte réseau pour l'entrée PSN (la sortie l'a déjà).
- Kratos réel : signe de l'orientation non vérifié ; 2 émetteurs sur un même groupe
  PSN se mélangent (limite connue).
- Place du bloc Scene dans l'Inspector (entre Video Output et Zones) à confirmer.

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
