/* dialog.h -- dialogues decrits par des tables : un seul jeu de widgets
 * partage par tous les dialogues (un seul est ouvert a la fois), et un
 * constructeur commun a la place d'une suite d'appels *_init().
 *
 * Usage : dialog_build(titre, largeur, hauteur, tableau, n, cmd_defaut) ;
 * ajuster au besoin dlg_item[i] (getter d'une liste...) ; dialog_run(). */
#ifndef TUI_DIALOG_H
#define TUI_DIALOG_H

#include "app.h"
#include "widgets.h"

#define DI_LABEL  0   /* text */
#define DI_BUTTON 1   /* text, w (0 = auto), arg = commande */
#define DI_INPUT  2   /* text = tampon (char *), w = largeur affichee, arg = max de caracteres */
#define DI_LIST   3   /* w, arg = hauteur ; getter/compte a poser apres dialog_build() */
#define DI_CHECK  4   /* text, arg = etat initial */

typedef struct {
    u8 kind, x, y, w;
    const char *text;
    u8 arg;
} TDlgItem;

typedef union {
    TLabel    label;
    TButton   button;
    TInputLine input;
    TListBox  list;
    TCheckBox check;
} TDlgWidget;

#define DLG_MAX 10

extern TWindow    dlg_win;
extern TDlgWidget dlg_item[DLG_MAX];

/* Construit dlg_win (modal, centre). Les champs de saisie sont vides
 * (input_init) : ecrire leur tampon puis input_set_text() APRES l'appel. */
void dialog_build(const char *title, u8 w, u8 h, const TDlgItem *items, u8 n,
                  u8 default_cmd);
#define dialog_run() app_exec(&dlg_win)

#endif
