/* tview.h -- vue de base, groupe, dispatch.
 *
 * Polymorphisme C : chaque widget embarque un TView en PREMIER membre et
 * pointe vers une table const de fonctions (TViewOps) partagee par toutes ses
 * instances. Coordonnees relatives au parent ; un enfant de fenetre a (0,0)
 * sur le coin haut-gauche du cadre.
 *
 * Contrat de handle() : renvoie EVENT_HANDLED s'il consomme l'evenement,
 * EVENT_NOT_HANDLED sinon -- l'evenement remonte alors au parent, eventuellement
 * TRANSFORME (un bouton reecrit une touche en EV_COMMAND avant de remonter). */
#ifndef TUI_TVIEW_H
#define TUI_TVIEW_H

#include "tui_config.h"
#include "event.h"

#define VF_VISIBLE   0x01
#define VF_FOCUSABLE 0x02
#define VF_FOCUSED   0x04   /* focus DANS son groupe parent */
#define VF_DIRTY     0x08   /* a redessiner */
#define VF_CDIRTY    0x10   /* un descendant est a redessiner */
#define VF_FULL      0x20   /* le parent a efface la zone : tout redessiner (pas de dessin partiel) */

#define NO_FOCUS 0xFF

struct TView;

typedef struct TViewOps {
    void (*draw)(struct TView *self);
    u8   (*handle)(struct TView *self, TEvent *ev);
} TViewOps;

typedef struct TView {
    u8 x, y, w, h;
    u8 flags;
    struct TView *parent;
    const TViewOps *ops;
} TView;

/* Groupe : conteneur d'enfants. Statique (pas de malloc). */
typedef struct TGroup {
    TView v;
    TView *child[TUI_MAX_CHILDREN];
    u8 count;
    u8 focus;                /* indice de l'enfant focus, ou NO_FOCUS */
} TGroup;

void view_init(TView *v, u8 x, u8 y, u8 w, u8 h, const TViewOps *ops, u8 flags);
/* Marque v (et la chaine de ses parents) a redessiner. */
void view_invalidate(TView *v);
/* Coin haut-gauche ABSOLU a l'ecran. */
void view_abs(const TView *v, u8 *ax, u8 *ay);
/* 1 si v a le focus jusqu'a la racine. */
u8   view_focused(const TView *v);
/* 1 si la cellule ecran (x,y) est dans v. */
u8   view_hit(const TView *v, u8 x, u8 y);
/* Redessine v s'il est sale (ou contient du sale). */
void view_draw(TView *v);

void group_init(TGroup *g, u8 x, u8 y, u8 w, u8 h, const TViewOps *ops, u8 flags);
u8   group_insert(TGroup *g, TView *v);      /* 0 si plein */
void group_remove(TGroup *g, TView *v);
void group_set_focus(TGroup *g, u8 idx);
void group_focus_view(TGroup *g, TView *v);
void group_focus_step(TGroup *g, u8 forward);
/* Corps generiques, reutilisables par les groupes specialises. */
void group_draw_children(TGroup *g, u8 force);
u8   group_handle(TView *self, TEvent *ev);

#endif
