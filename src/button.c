/* button.c -- TButton : "< texte >", Entree/Espace/clic emet la commande. */

#include <string.h>
#include "widgets.h"
#include "screen.h"
#include "keyboard.h"

static void button_draw(TView *v)
{
    TButton *b = (TButton *)v;
    u8 ax, ay, n, pad;
    u8 attr = (v->flags & VF_FOCUSED) && view_focused(v) ? ATTR_INVERSE : ATTR_NORMAL;
    view_abs(v, &ax, &ay);
    n = (u8)strlen(b->text);
    pad = (u8)((v->w - 2 > n) ? (v->w - 2 - n) / 2 : 0);
    scr_fill(ax, ay, v->w, 1, ' ', attr);
    scr_putc(ax, ay, '<', attr);
    scr_putc(ax + v->w - 1, ay, '>', attr);
    scr_puts(ax + 1 + pad, ay, b->text, attr);
    v->flags &= ~VF_DIRTY;
}

static u8 button_handle(TView *v, TEvent *ev)
{
    TButton *b = (TButton *)v;
    if ((ev->type == EV_KEY && (ev->key == KEY_ENTER || ev->key == ' '))
#if TUI_MOUSE
        || ev->type == EV_MOUSE_DOWN
#endif
        ) {
        ev->type = EV_COMMAND;      /* transforme, puis remonte au parent */
        ev->cmd = b->cmd;
        return EVENT_NOT_HANDLED;
    }
    return EVENT_NOT_HANDLED;
}

static const TViewOps button_ops = { button_draw, button_handle };

void button_init(TButton *b, u8 x, u8 y, u8 w, const char *text, u8 cmd)
{
    u8 min = (u8)strlen(text) + 4;
    view_init(&b->v, x, y, w < min ? min : w, 1, &button_ops, VF_FOCUSABLE);
    b->text = text;
    b->cmd = cmd;
}
