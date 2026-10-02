/* window.c -- TWindow, dialogues modaux, msgbox. */

#include <string.h>
#include "window.h"
#include "widgets.h"
#include "screen.h"
#include "keyboard.h"
#include "app.h"

static void window_draw(TView *v)
{
    TWindow *w = (TWindow *)v;
    u8 ax, ay, tl;
    if (v->flags & VF_DIRTY) {
        view_abs(v, &ax, &ay);
        scr_box(ax, ay, v->w, v->h);
        if (w->title && v->w > 6) {
            tl = (u8)strlen(w->title);
            if (tl > v->w - 6)
                tl = (u8)(v->w - 6);
            scr_putc(ax + 2, ay, ' ', ATTR_INVERSE);
            scr_putsw(ax + 3, ay, w->title, tl, ATTR_INVERSE);
            scr_putc(ax + 3 + tl, ay, ' ', ATTR_INVERSE);
        }
        group_draw_children(&w->g, 1);
    } else {
        group_draw_children(&w->g, 0);
    }
}

static u8 window_handle(TView *v, TEvent *ev)
{
    TWindow *w = (TWindow *)v;
    if (group_handle(v, ev) == EVENT_HANDLED)
        return EVENT_HANDLED;
    if (ev->type == EV_KEY) {
        if (ev->key == KEY_ENTER && w->default_cmd != CM_NONE) {
            ev->type = EV_COMMAND;
            ev->cmd = w->default_cmd;
        } else if (ev->key == KEY_ESC && (w->wflags & WF_MODAL)) {
            ev->type = EV_COMMAND;
            ev->cmd = CM_CANCEL;
        }
    }
    if (ev->type == EV_COMMAND && (w->wflags & WF_MODAL) &&
        ev->cmd >= CM_OK && ev->cmd <= CM_NO) {
        w->result = ev->cmd;
        return EVENT_HANDLED;
    }
    return EVENT_NOT_HANDLED;
}

static const TViewOps window_ops = { window_draw, window_handle };

void window_init(TWindow *w, u8 x, u8 y, u8 width, u8 height,
                 const char *title, u8 wflags)
{
    group_init(&w->g, x, y, width, height, &window_ops, VF_FOCUSABLE);
    w->title = title;
    w->wflags = wflags;
    w->result = 0;
    w->default_cmd = CM_NONE;
}

u8 window_add(TWindow *w, TView *child)
{
    return group_insert(&w->g, child);
}

void window_center(TWindow *w)
{
    w->g.v.x = (u8)((scr_cols - w->g.v.w) / 2);
    w->g.v.y = (u8)((TUI_DESK_ROWS - w->g.v.h) / 2);
}

void window_set_default(TWindow *w, u8 cmd)
{
    w->default_cmd = cmd;
}
