/* progress.c -- TProgress : barre de progression. */

#include "widgets.h"
#include "screen.h"

static void progress_draw(TView *v)
{
    TProgress *p = (TProgress *)v;
    u8 ax, ay, i, filled;
    view_abs(v, &ax, &ay);
    filled = p->max ? (u8)(((uint32_t)p->value * v->w) / p->max) : 0;
    for (i = 0; i < v->w; ++i) {
        if (i < filled)
            scr_putc(ax + i, ay, ' ', ATTR_INVERSE);
        else
            scr_putc(ax + i, ay, '.', ATTR_NORMAL);
    }
    v->flags &= ~VF_DIRTY;
}

static u8 progress_handle(TView *v, TEvent *ev)
{
    (void)v; (void)ev;
    return EVENT_NOT_HANDLED;
}

static const TViewOps progress_ops = { progress_draw, progress_handle };

void progress_init(TProgress *p, u8 x, u8 y, u8 w, u16 max)
{
    view_init(&p->v, x, y, w, 1, &progress_ops, 0);
    p->value = 0;
    p->max = max;
}

void progress_set(TProgress *p, u16 value)
{
    if (value > p->max)
        value = p->max;
    if (value != p->value) {
        p->value = value;
        view_invalidate(&p->v);
    }
}
