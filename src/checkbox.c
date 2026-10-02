/* checkbox.c -- TCheckBox. */

#include <string.h>
#include "widgets.h"
#include "screen.h"
#include "keyboard.h"

static void checkbox_draw(TView *v)
{
    TCheckBox *c = (TCheckBox *)v;
    u8 ax, ay;
    u8 attr = view_focused(v) ? ATTR_INVERSE : ATTR_NORMAL;
    view_abs(v, &ax, &ay);
    scr_putc(ax, ay, '(', ATTR_NORMAL);
    scr_putc(ax + 1, ay, c->checked ? 'x' : ' ', attr);
    scr_putc(ax + 2, ay, ')', ATTR_NORMAL);
    scr_putc(ax + 3, ay, ' ', ATTR_NORMAL);
    scr_putsw(ax + 4, ay, c->text, (u8)(v->w - 4), ATTR_NORMAL);
    v->flags &= ~VF_DIRTY;
}

static u8 checkbox_handle(TView *v, TEvent *ev)
{
    TCheckBox *c = (TCheckBox *)v;
    if ((ev->type == EV_KEY && ev->key == ' ')
#if TUI_MOUSE
        || ev->type == EV_MOUSE_DOWN
#endif
        ) {
        c->checked = (u8)!c->checked;
        view_invalidate(v);
        return EVENT_HANDLED;
    }
    return EVENT_NOT_HANDLED;
}

static const TViewOps checkbox_ops = { checkbox_draw, checkbox_handle };

void checkbox_init(TCheckBox *c, u8 x, u8 y, const char *text, u8 checked)
{
    view_init(&c->v, x, y, (u8)(strlen(text) + 4), 1, &checkbox_ops, VF_FOCUSABLE);
    c->text = text;
    c->checked = checked;
}
