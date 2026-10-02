/* menu.h -- barre de menu en haut d'ecran et menus deroulants.
 *
 * Ouverture : Echap (quand aucune vue ne la consomme). Fleches gauche/droite
 * changent de menu, haut/bas d'article, Entree valide, Echap referme.
 * Dans un menu ouvert, taper l'initiale d'un article le declenche.
 * Un article peut porter un raccourci Ctrl-lettre (key = KEY_CTRL('S')),
 * actif partout sauf dans un dialogue modal. Attention : $08/$09/$0A/$0B/
 * $0D/$15/$1B sont les fleches, Tab, Entree, Echap -- inutilisables. */
#ifndef TUI_MENU_H
#define TUI_MENU_H

#include "tview.h"

typedef struct {
    const char *text;        /* "-" : separateur */
    u8 cmd;
    u8 key;                  /* raccourci Ctrl, 0 = aucun */
} TMenuItem;

typedef struct {
    const char *title;
    const TMenuItem *items;
    u8 count;
} TMenu;

typedef struct {
    TView v;
    const TMenu *menus;
    u8 count;
} TMenuBar;

void menubar_init(TMenuBar *mb, const TMenu *menus, u8 count);
/* Commande associee au raccourci clavier, ou CM_NONE. */
u8   menubar_find_key(const TMenuBar *mb, u8 key);
/* Boucle modale : renvoie la commande choisie, ou CM_NONE (Echap). */
u8   menubar_run(TMenuBar *mb);
/* Idem, en ouvrant directement le menu m. */
u8   menubar_open(TMenuBar *mb, u8 m);
/* Indice du menu dont le titre couvre la colonne x, ou 0xFF. */
u8   menubar_title_at(const TMenuBar *mb, u8 x);

/* Table de rebond utilisee par app.c pour parler a ce module SANS le
 * nommer : ca laisse menu.o hors du lien pour une appli qui n'appelle
 * jamais menubar_init() (le lieur ne fait de l'elagage qu'au fichier objet
 * entier, pas par fonction -- une reference directe de app.c a menubar_run()
 * suffirait a l'inclure toujours). menubar_init() enregistre cette table
 * aupres de app.c via app_set_menu_hooks() ; app.c ne connait que le type. */
typedef struct {
    u8 (*find_key)(const TMenuBar *mb, u8 key);
    u8 (*run)(TMenuBar *mb);
    u8 (*open)(TMenuBar *mb, u8 m);
    u8 (*title_at)(const TMenuBar *mb, u8 x);
} TMenuHooks;

extern const TMenuHooks menu_hooks;

#endif
