/* radio.c -- TRadioGroup : groupe de boutons radio (choix exclusif), une
 * ligne par option. Une seule vue focusable pour tout le groupe (comme
 * TListBox) plutot qu'un TView par bouton : Haut/Bas deplacent ET
 * selectionnent (pas d'etape "survol" separee -- inutile pour de petits
 * groupes a options fixes). */

#include "widgets.h"
#include "screen.h"
#include "keyboard.h"

static void radiogroup_draw(TView *v)
{
    TRadioGroup *r = (TRadioGroup *)v;
    u8 ax, ay, i;
    u8 focused = view_focused(v);
    view_abs(v, &ax, &ay);
    for (i = 0; i < r->count; ++i) {
        u8 sel = (u8)(i == r->sel);
        u8 attr = (sel && focused) ? ATTR_INVERSE : ATTR_NORMAL;
        scr_putc(ax, (u8)(ay + i), '(', ATTR_NORMAL);
        scr_putc((u8)(ax + 1), (u8)(ay + i), sel ? 'o' : ' ', attr);
        scr_putc((u8)(ax + 2), (u8)(ay + i), ')', ATTR_NORMAL);
        scr_putc((u8)(ax + 3), (u8)(ay + i), ' ', attr);
        scr_putsw((u8)(ax + 4), (u8)(ay + i), r->labels[i], (u8)(v->w - 4), attr);
    }
    v->flags &= ~VF_DIRTY;
}

void radiogroup_select(TRadioGroup *r, u8 sel)
{
    if (r->count == 0)
        return;
    r->sel = sel < r->count ? sel : (u8)(r->count - 1);
    view_invalidate(&r->v);
}

static u8 radiogroup_handle(TView *v, TEvent *ev)
{
    TRadioGroup *r = (TRadioGroup *)v;
#if TUI_MOUSE
    if (ev->type == EV_MOUSE_DOWN) {
        u8 ax, ay, idx;
        view_abs(v, &ax, &ay);
        idx = (u8)(ev->mouse_y - ay);
        if (idx < r->count)
            radiogroup_select(r, idx);
        return EVENT_HANDLED;
    }
#endif
    if (ev->type != EV_KEY)
        return EVENT_NOT_HANDLED;
    switch (ev->key) {
    case KEY_UP:
        if (r->sel > 0)
            radiogroup_select(r, (u8)(r->sel - 1));
        return EVENT_HANDLED;
    case KEY_DOWN:
        if ((u8)(r->sel + 1) < r->count)
            radiogroup_select(r, (u8)(r->sel + 1));
        return EVENT_HANDLED;
    }
    return EVENT_NOT_HANDLED;
}

static const TViewOps radiogroup_ops = { radiogroup_draw, radiogroup_handle };

void radiogroup_init(TRadioGroup *r, u8 x, u8 y, u8 w,
                     const char * const *labels, u8 count, u8 sel)
{
    view_init(&r->v, x, y, w, count, &radiogroup_ops, VF_FOCUSABLE);
    r->labels = labels;
    r->count = count;
    r->sel = sel < count ? sel : 0;
}
