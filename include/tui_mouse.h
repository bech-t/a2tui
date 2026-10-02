/* tui_mouse.h -- souris (module isole ; TUI_MOUSE=0 : stub sans driver).
 *
 * Sur Apple II : driver standard de cc65 (a2.stdmou : carte souris Apple II,
 * port souris du //c), lie statiquement. Coordonnees rendues en cellules texte.
 * Bouton gauche seul ; buttons & 1 = bouton enfonce.
 * Le pointeur est dessine par screen.c (scr_mouse_move/scr_mouse_show). */
#ifndef TUI_MOUSE_H
#define TUI_MOUSE_H

#include "tui_config.h"

struct TEvent;

u8   mouse_init(void);               /* apres scr_init() ; 1 si une souris est utilisable */
void mouse_done(void);
u8   mouse_present(void);
/* 1 si ev a ete rempli (EV_MOUSE_DOWN / _UP / _MOVE, en cellules). */
u8   mouse_poll(struct TEvent *ev);

#ifdef TUI_HOST
/* Injecte un evenement souris (version hote). type = EV_MOUSE_*. */
void tui_host_push_mouse(u8 type, u8 x, u8 y);
#endif

#endif
