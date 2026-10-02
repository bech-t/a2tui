# Suite du projet

État : la bibliothèque TUI v1 est en place (rendu, événements, vues, menus,
dialogues) et validée. Ce document note les deux applications
suivantes ; les choix ci-dessous sont des **propositions à valider**.

## 0. Compléments TUI (issus de l'éditeur)

- Widgets (glyphes `G_CHECK`, `G_TRACK`, flèches déjà prêts) : case à cocher, barre de défilement, barre de progression, zone de texte multiligne
  (base de l'éditeur), barre d'état à champs.
- `TListBox` : Home/End, défilement à la molette/barre à la souris ; sélection multiple
  (usage futur).
- Curseur clignotant logiciel (le jeu alternatif n'a pas de flash).
- Optimisation taille/vitesse (voir README) : `screen.c` en assembleur d'abord.
- Souris : double-clic, glisser (barre de défilement, sélection de texte pour
  l'éditeur), déplacement de fenêtres ; test sur carte souris réelle.
- Save-under des dialogues si le redessin complet se voit.

## 1. Éditeur de texte (`apps/edit`) -- v1 faite

Voir le README. Reste à faire, par ordre d'intérêt :

- **Plus de place pour le texte.** 8 Ko (11 Ko sans souris) est le vrai point
  faible. Pistes : réduire encore le code (allègement de `editor.c`, `menu.c`,
  `widgets.c`) ; libérer `/RAM` sur 128 Ko et y loger le tampon (demande de
  démonter proprement le volume ProDOS) ; ou éditer par fenêtre glissante sur un
  fichier temporaire dans `/RAM`.
- **Vitesse** : le défilement ligne à ligne coûte ~0,4 s (redessin de ~20 lignes
  à 1 MHz). Le profil montre un coût réparti (copie du flush, appels cc65) : gagner
  encore demande de l'assembleur pour le rendu d'une ligne de texte.
- Annuler (Undo), mode Écrasement, largeur de tabulation réglable, impression,
  recherche de mots entiers, curseur clignotant (le jeu alternatif n'a pas de flash).
- Tester sur vrai matériel : disque, souris, touches Pomme.

## Questions ouvertes

- Éditeur : fichiers > 20 Ko supportés (fenêtre glissante sur disque) ou
  limite mémoire acceptée en v1 ?
