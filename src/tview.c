/* tview.c -- vue de base, groupe, focus, dispatch. */

#include "tview.h"
#include "keyboard.h"

void view_init(TView *v, u8 x, u8 y, u8 w, u8 h, const TViewOps *ops, u8 flags)
{
    v->x = x; v->y = y; v->w = w; v->h = h;
    v->flags = flags | VF_VISIBLE | VF_DIRTY;
    v->parent = 0;
    v->ops = ops;
}

void view_invalidate(TView *v)
{
    v->flags |= VF_DIRTY;
    for (v = v->parent; v; v = v->parent)
        v->flags |= VF_CDIRTY;
}

void view_abs(const TView *v, u8 *ax, u8 *ay)
{
    u8 x = 0, y = 0;
    for (; v; v = v->parent) {
        x += v->x;
        y += v->y;
    }
    *ax = x;
    *ay = y;
}

u8 view_focused(const TView *v)
{
    for (; v->parent; v = v->parent)
        if (!(v->flags & VF_FOCUSED))
            return 0;
    return 1;
}

u8 view_hit(const TView *v, u8 x, u8 y)
{
    u8 ax, ay;
    view_abs(v, &ax, &ay);
    return x >= ax && x < (u8)(ax + v->w) && y >= ay && y < (u8)(ay + v->h);
}

void view_draw(TView *v)
{
    if ((v->flags & VF_VISIBLE) && (v->flags & (VF_DIRTY | VF_CDIRTY)))
        v->ops->draw(v);
}

/* --- Groupe ------------------------------------------------------------ */

void group_init(TGroup *g, u8 x, u8 y, u8 w, u8 h, const TViewOps *ops, u8 flags)
{
    view_init(&g->v, x, y, w, h, ops, flags);
    g->count = 0;
    g->focus = NO_FOCUS;
}

static u8 index_of(const TGroup *g, const TView *v)
{
    u8 i;
    for (i = 0; i < g->count; ++i)
        if (g->child[i] == v)
            return i;
    return NO_FOCUS;
}

u8 group_insert(TGroup *g, TView *v)
{
    if (g->count >= TUI_MAX_CHILDREN)
        return 0;
    g->child[g->count++] = v;
    v->parent = &g->v;
    view_invalidate(v);
    if (g->focus == NO_FOCUS && (v->flags & VF_FOCUSABLE))
        group_set_focus(g, (u8)(g->count - 1));
    return 1;
}

void group_remove(TGroup *g, TView *v)
{
    u8 i = index_of(g, v);
    if (i == NO_FOCUS)
        return;
    v->flags &= ~VF_FOCUSED;
    v->parent = 0;
    for (; i + 1 < g->count; ++i)
        g->child[i] = g->child[i + 1];
    --g->count;
    /* Le focus est recalcule : le dernier enfant focalisable prend la main
     * (le plus recemment insere = celui du dessus). */
    g->focus = NO_FOCUS;
    for (i = 0; i < g->count; ++i)
        g->child[i]->flags &= ~VF_FOCUSED;
    for (i = g->count; i-- > 0;)
        if (g->child[i]->flags & VF_FOCUSABLE) {
            group_set_focus(g, i);
            break;
        }
    view_invalidate(&g->v);
}

void group_set_focus(TGroup *g, u8 idx)
{
    TView *c;
    if (g->focus == idx)
        return;
    if (g->focus != NO_FOCUS && g->focus < g->count) {
        c = g->child[g->focus];
        c->flags &= ~VF_FOCUSED;
        view_invalidate(c);
    }
    g->focus = idx;
    if (idx != NO_FOCUS) {
        c = g->child[idx];
        c->flags |= VF_FOCUSED;
        view_invalidate(c);
    }
}

void group_focus_view(TGroup *g, TView *v)
{
    u8 i = index_of(g, v);
    if (i != NO_FOCUS)
        group_set_focus(g, i);
}

void group_focus_step(TGroup *g, u8 forward)
{
    u8 i, n;
    if (g->count == 0)
        return;
    i = (g->focus == NO_FOCUS) ? (forward ? g->count - 1 : 0) : g->focus;
    for (n = 0; n < g->count; ++n) {
        i = forward ? (u8)((i + 1) % g->count)
                    : (u8)(i ? i - 1 : g->count - 1);
        if ((g->child[i]->flags & (VF_FOCUSABLE | VF_VISIBLE)) ==
            (VF_FOCUSABLE | VF_VISIBLE)) {
            group_set_focus(g, i);
            return;
        }
    }
}

void group_draw_children(TGroup *g, u8 force)
{
    u8 i;
    TView *c;
    for (i = 0; i < g->count; ++i) {
        c = g->child[i];
        if (force)
            c->flags |= VF_DIRTY | VF_FULL;
        view_draw(c);
    }
    g->v.flags &= ~(VF_DIRTY | VF_CDIRTY);
}

/* Dispatch clavier : l'enfant focus d'abord, puis navigation du focus si
 * personne n'a consomme la touche. Les commandes traversent sans arret. */
u8 group_handle(TView *self, TEvent *ev)
{
    TGroup *g = (TGroup *)self;
#if TUI_MOUSE
    if (ev->type == EV_MOUSE_DOWN) {
        u8 i;
        /* Le dernier insere est dessine au-dessus : on le teste en premier. */
        for (i = g->count; i-- > 0;) {
            TView *c = g->child[i];
            if ((c->flags & VF_VISIBLE) && view_hit(c, ev->mouse_x, ev->mouse_y)) {
                if (c->flags & VF_FOCUSABLE)
                    group_set_focus(g, i);
                return c->ops->handle(c, ev);
            }
        }
        return EVENT_NOT_HANDLED;
    }
    /* Glisser : mouvement et relachement vont a l'enfant focus. */
    if ((ev->type == EV_MOUSE_MOVE || ev->type == EV_MOUSE_UP) && g->focus != NO_FOCUS) {
        TView *c = g->child[g->focus];
        return c->ops->handle(c, ev);
    }
#endif
    if (ev->type == EV_KEY && g->focus != NO_FOCUS) {
        TView *c = g->child[g->focus];
        if (c->ops->handle(c, ev) == EVENT_HANDLED)
            return EVENT_HANDLED;
    }
    if (ev->type == EV_KEY) {
        switch (ev->key) {
        case KEY_TAB:
        case KEY_DOWN:
        case KEY_RIGHT:
            group_focus_step(g, 1);
            return EVENT_HANDLED;
        case KEY_UP:
        case KEY_LEFT:
            group_focus_step(g, 0);
            return EVENT_HANDLED;
        }
    }
    return EVENT_NOT_HANDLED;
}
