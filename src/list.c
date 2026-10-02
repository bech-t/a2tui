/* list.c -- TListBox : liste scrollable, selection clavier ou souris. */

#include "widgets.h"
#include "screen.h"
#include "keyboard.h"

const char *list_strings_get(void *ctx, u16 index)
{
    return ((const char *const *)ctx)[index];
}

static void list_draw(TView *v)
{
    TListBox *l = (TListBox *)v;
    u8 ax, ay, r;
    u16 i;
    u8 focused = view_focused(v);
    view_abs(v, &ax, &ay);
    for (r = 0; r < v->h; ++r) {
        i = l->top + r;
        if (i >= l->count) {
            scr_fill(ax, ay + r, v->w, 1, ' ', ATTR_NORMAL);
        } else if (i == l->sel) {
            scr_putc(ax, ay + r, '>', ATTR_NORMAL);
            scr_putsw(ax + 1, ay + r, l->get(l->ctx, i), (u8)(v->w - 1),
                      focused ? ATTR_INVERSE : ATTR_NORMAL);
        } else {
            scr_putc(ax, ay + r, ' ', ATTR_NORMAL);
            scr_putsw(ax + 1, ay + r, l->get(l->ctx, i), (u8)(v->w - 1), ATTR_NORMAL);
        }
    }
    v->flags &= ~VF_DIRTY;
}

static void list_fix_top(TListBox *l)
{
    if (l->sel < l->top)
        l->top = l->sel;
    else if (l->sel >= l->top + l->v.h)
        l->top = l->sel - l->v.h + 1;
}

void list_select(TListBox *l, u16 index)
{
    if (l->count == 0)
        return;
    l->sel = index < l->count ? index : (u16)(l->count - 1);
    list_fix_top(l);
    view_invalidate(&l->v);
}

static u8 list_handle(TView *v, TEvent *ev)
{
    TListBox *l = (TListBox *)v;
#if TUI_MOUSE
    if (ev->type == EV_MOUSE_DOWN) {
        u8 ax, ay;
        u16 idx;
        view_abs(v, &ax, &ay);
        idx = l->top + (ev->mouse_y - ay);
        if (idx >= l->count)
            return EVENT_HANDLED;
        if (idx == l->sel && l->cmd != CM_NONE) {   /* second clic : valide */
            ev->type = EV_COMMAND;
            ev->cmd = l->cmd;
            return EVENT_NOT_HANDLED;
        }
        list_select(l, idx);
        return EVENT_HANDLED;
    }
#endif
    if (ev->type != EV_KEY)
        return EVENT_NOT_HANDLED;
    switch (ev->key) {
    case KEY_UP:
        if (l->sel > 0)
            list_select(l, l->sel - 1);
        return EVENT_HANDLED;
    case KEY_DOWN:
        if (l->sel + 1 < l->count)
            list_select(l, l->sel + 1);
        return EVENT_HANDLED;
    case KEY_CTRL('W'):                     /* page precedente */
        list_select(l, l->sel > l->v.h ? l->sel - l->v.h : 0);
        return EVENT_HANDLED;
    case KEY_CTRL('Z'):                     /* page suivante */
        list_select(l, l->sel + l->v.h);
        return EVENT_HANDLED;
    case KEY_ENTER:
        if (l->cmd != CM_NONE) {
            ev->type = EV_COMMAND;
            ev->cmd = l->cmd;
        }
        return EVENT_NOT_HANDLED;
    }
    return EVENT_NOT_HANDLED;
}

static const TViewOps list_ops = { list_draw, list_handle };

void list_init(TListBox *l, u8 x, u8 y, u8 w, u8 h,
               TListGet get, void *ctx, u16 count, u8 cmd)
{
    view_init(&l->v, x, y, w, h, &list_ops, VF_FOCUSABLE);
    l->get = get;
    l->ctx = ctx;
    l->count = count;
    l->sel = l->top = 0;
    l->cmd = cmd;
}

void list_set_count(TListBox *l, u16 count)
{
    l->count = count;
    l->sel = l->top = 0;
    view_invalidate(&l->v);
}
