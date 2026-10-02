/* event.h -- evenements normalises (clavier, souris, commandes). */
#ifndef TUI_EVENT_H
#define TUI_EVENT_H

#include "tui_config.h"

#define EV_NONE        0
#define EV_KEY         1
#define EV_COMMAND     2   /* emis par un widget (bouton, menu...) */
#define EV_MOUSE_DOWN  3
#define EV_MOUSE_UP    4
#define EV_MOUSE_MOVE  5

/* Retours de handle_event() */
#define EVENT_NOT_HANDLED 0   /* l'evenement (eventuellement transforme) remonte au parent */
#define EVENT_HANDLED     1

/* Commandes standard. Les commandes applicatives commencent a CM_USER. */
#define CM_NONE    0
#define CM_OK      1
#define CM_CANCEL  2
#define CM_YES     3
#define CM_NO      4
#define CM_QUIT    5
#define CM_USER    32

typedef struct TEvent {
    u8 type;
    u8 key;                 /* EV_KEY : code ASCII 7 bits */
    u8 cmd;                 /* EV_COMMAND */
    u8 mouse_x, mouse_y;    /* colonne/ligne texte */
    u8 buttons;             /* souris : bit 0 = bouton gauche */
    u8 mods;                /* EV_KEY : MOD_OPEN_APPLE / MOD_CLOSED_APPLE (keyboard.h) */
} TEvent;

/* Remplit *ev (EV_NONE si rien). Un evenement au plus par appel. */
void event_poll(TEvent *ev);

#endif
