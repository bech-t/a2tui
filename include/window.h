/* window.h -- fenetres, boites de dialogue modales, msgbox. */
#ifndef TUI_WINDOW_H
#define TUI_WINDOW_H

#include "tview.h"

#define WF_MODAL 0x01        /* dialogue : Echap = Annuler, OK/Annuler/Oui/Non le ferment */

typedef struct TWindow {
    TGroup g;
    const char *title;
    u8 wflags;
    u8 result;               /* commande de fermeture d'un dialogue modal */
    u8 default_cmd;          /* emise sur Entree si aucun enfant ne la prend */
} TWindow;

/* (x,y) relatifs au bureau. Cadre compris dans w x h ; les enfants sont
 * positionnes a partir du coin haut-gauche du cadre (donc >= 1). */
void window_init(TWindow *w, u8 x, u8 y, u8 width, u8 height,
                 const char *title, u8 wflags);
u8   window_add(TWindow *w, TView *child);
void window_center(TWindow *w);
void window_set_default(TWindow *w, u8 cmd);

/* Boite de message prete a l'emploi. text peut contenir des '\n' (4 lignes
 * max). Renvoie CM_OK, CM_CANCEL, CM_YES ou CM_NO. */
#define MB_OK       0
#define MB_OKCANCEL 1
#define MB_YESNO    2
#define MB_YESNOCANCEL 3
/* Utilise le jeu de widgets partage de dialog.h : ne pas l'appeler pendant
 * qu'un autre dialogue (dialog_run) est ouvert. */
u8 msgbox(const char *title, const char *text, u8 kind);

#endif
