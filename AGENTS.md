# Travailler sur le backlog

Guide pour un agent (ou une personne) qui prend une issue. Chaque issue unitaire contient déjà ses fichiers à
modifier et sa definition of done : ce document ne décrit que ce qui est commun à toutes.

## Choisir une issue

Prendre une issue ouverte, non bloquée, qui n'est pas une issue de suivi :

```bash
gh issue list --search 'is:open label:"statut:prêt" -label:ne-pas-prendre' --limit 50
```

Filtrer ensuite selon le modèle et l'effort disponibles :

| Label | Sens |
|---|---|
| `type:carte`, `type:relique`, `type:potion`, `type:monstre`, `type:élite`, `type:évènement`, `type:ancien` | Un seul élément |
| `type:mécanique`, `type:boss`, `type:système` | Tâche structurante, aussi marquée `grosse-tâche` |
| `type:suivi` + `ne-pas-prendre` | Issue parente : ne jamais la prendre, le travail est dans ses sous-issues |
| `agent:haiku`, `agent:sonnet`, `agent:opus` | Modèle conseillé |
| `effort:faible`, `effort:moyen`, `effort:élevé` | Niveau de réflexion conseillé |
| `taille:XS` à `taille:XL` | Volume de travail |
| `statut:prêt`, `statut:bloqué` | Tenu à jour par `.github/workflows/backlog.yml` à chaque fermeture d'issue |
| `merge:solo`, `merge:vérif-écran` | Qui merge (voir plus bas) |
| `héros:…`, `acte:…` | Édition et acte concernés |

Exemple, les petites tâches pour Haiku : `gh issue list --search 'is:open label:"statut:prêt" label:"agent:haiku"'`.

## Travailler à plusieurs en parallèle

Deux agents sur le même type d'élément ajoutent des lignes aux mêmes endroits (`CardId`, le `switch` de
`resolve_effect`, les listes de tirage). Répartir de préférence un agent par type (`type:carte`,
`type:relique`, `type:monstre`…) ou par héros. Quand un conflit arrive quand même : fusionner `main` dans la
branche et garder les deux côtés.

## Règles d'une PR

- Une branche `feat/<numéro>-<slug>` partie de `main`, une PR dont la description contient `Closes #<numéro>`.
- Ne rien implémenter d'autre que l'issue. Un manque découvert en route devient une nouvelle issue.
- Ne pas committer `site/`, `AVANCEMENT.md` ni `VERSION` : le workflow de release les régénère.
- Avant de pousser : `make test test-e2e` et
  `uv run --with pytest --with pillow --with numpy --with markdown pytest scripts`.

## Merger

`merge:solo` : la CI fait foi, aucune relecture humaine.

```bash
gh pr update-branch <pr>               # obligatoire si main a avancé depuis la dernière CI
gh pr checks <pr> --watch --fail-fast
gh pr merge <pr> --squash --delete-branch
```

Mettre la branche à jour avant de merger n'est pas une formalité : deux PR vertes chacune de leur côté peuvent
casser `main` une fois réunies. Le dépôt est privé sur un plan gratuit, GitHub ne peut donc pas l'imposer par
une protection de branche.

`merge:vérif-écran` : la PR change ce que la GBA affiche. Ne pas merger seul. Joindre à la PR des captures
prises avec l'émulateur sans fenêtre (ci-dessous) et laisser la session principale valider.

## Voir l'écran sans ouvrir mGBA

```bash
uv run scripts/setup_headless_mgba.py     # une seule fois : compile l'émulateur dans tools/mgba/
make ironclad
uv run scripts/gba_headless.py slay_ironclad.gba wait:300 A:90 A:90 shot:combat.png
```

Étapes disponibles : un bouton (`A B L R START SELECT UP DOWN LEFT RIGHT`, suivi de `:<images>` pour attendre
plus longtemps), `wait:<images>`, `shot:<fichier.png>`. Le jeu tourne à 60 images par seconde. Aucune
sauvegarde n'est lue ni écrite. Depuis Python : `from gba_headless import Gba`.
