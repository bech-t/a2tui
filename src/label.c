/* label.c -- TLabel : texte statique. */

#include <string.h>
#include "widgets.h"
#include "screen.h"

static void label_draw(TView *v)
{
    TLabel *l = (TLabel *)v;
    u8 ax, ay;
    view_abs(v, &ax, &ay);
    scr_putsw(ax, ay, l->text, v->w, ATTR_NORMAL);
    v->flags &= ~VF_DIRTY;
}

static u8 label_handle(TView *v, TEvent *ev)
{
    (void)v; (void)ev;
    return EVENT_NOT_HANDLED;
}

static const TViewOps label_ops = { label_draw, label_handle };

void label_init(TLabel *l, u8 x, u8 y, u8 w, const char *text)
{
    view_init(&l->v, x, y, w ? w : (u8)strlen(text), 1, &label_ops, 0);
    l->text = text;
}

void label_set(TLabel *l, const char *text)
{
    l->text = text;
    view_invalidate(&l->v);
}
