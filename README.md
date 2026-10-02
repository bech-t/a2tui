# a2tui

[English](README.en.md)

Bibliothèque C ([cc65](https://cc65.github.io/)) d'interface texte façon
Turbo Vision pour **Apple //e et //c** : menus déroulants, fenêtres, dialogues,
boutons, champs de saisie, listes, cases à cocher, barre de progression,
sélecteur de fichier, souris. 40 ou 80 colonnes, MouseText quand la machine le
permet, tout en mémoire statique (pas de `malloc`).

![Démo a2tui en 80 colonnes (Apple //e)](docs/demo.png)

Le dépôt contient aussi une démo et un **éditeur de texte** (`apps/edit`,
dans l'esprit de MS-DOS EDIT) qui exploite la RAM auxiliaire quand elle existe.

## Essayer

```
make            # build/a2tui.lib
make dsk        # build/tuidemo.dsk : disquette ProDOS bootable (la démo)
make edit-dsk   # build/edit.dsk : l'éditeur de texte
```

Prérequis : cc65, Java et AppleCommander ([`tools/ac/README.md`](tools/ac/README.md)).
Les disquettes se lancent sur un Apple //e ou //c, réel ou émulé.

## Documentation

Guide d'utilisation et notes techniques : [français](docs/usage.fr.md) ·
[English](docs/usage.en.md).

## Licence

MIT, voir [`LICENSE`](LICENSE). Les fichiers de `prodos/` sont ceux de
ProDOS 2.4.2 (libre) ; AppleCommander n'est pas inclus.
