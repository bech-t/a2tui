/* input.c -- TInputLine : champ de saisie mono-ligne avec curseur. */

#include <string.h>
#include "widgets.h"
#include "screen.h"
#include "keyboard.h"

static void input_draw(TView *v)
{
    TInputLine *in = (TInputLine *)v;
    u8 ax, ay, i, idx;
    u8 focused = view_focused(v);
    char c;
    view_abs(v, &ax, &ay);
    for (i = 0; i < v->w; ++i) {
        idx = in->scroll + i;
        c = (idx < in->len) ? in->buf[idx] : '.';
        if (focused && idx == in->pos)
            scr_putc(ax + i, ay, idx < in->len ? c : ' ', ATTR_FLASH);
        else
            scr_putc(ax + i, ay, c, ATTR_NORMAL);
    }
    v->flags &= ~VF_DIRTY;
}

static void input_fix_scroll(TInputLine *in)
{
    if (in->pos < in->scroll)
        in->scroll = in->pos;
    else if (in->pos >= in->scroll + in->v.w)
        in->scroll = (u8)(in->pos - in->v.w + 1);
}

static void input_erase(TInputLine *in, u8 at)
{
    memmove(in->buf + at, in->buf + at + 1, (size_t)(in->len - at));
    --in->len;
}

static u8 input_handle(TView *v, TEvent *ev)
{
    TInputLine *in = (TInputLine *)v;
    u8 k;
#if TUI_MOUSE
    if (ev->type == EV_MOUSE_DOWN) {
        u8 ax, ay;
        view_abs(v, &ax, &ay);
        in->pos = (u8)(in->scroll + (ev->mouse_x - ax));
        if (in->pos > in->len)
            in->pos = in->len;
        view_invalidate(v);
        return EVENT_HANDLED;
    }
#endif
    if (ev->type != EV_KEY)
        return EVENT_NOT_HANDLED;
    k = ev->key;
    if (k >= 0x20 && k < 0x7F) {
        if (in->len < in->max) {
            memmove(in->buf + in->pos + 1, in->buf + in->pos,
                    (size_t)(in->len - in->pos + 1));
            in->buf[in->pos++] = (char)k;
            ++in->len;
        }
    } else if (k == KEY_DEL || (TUI_LEFT_IS_BS && k == KEY_LEFT && in->pos == in->len)) {
        if (in->pos > 0)
            input_erase(in, --in->pos);
    } else if (k == KEY_LEFT) {
        if (in->pos > 0)
            --in->pos;
    } else if (k == KEY_RIGHT) {
        if (in->pos < in->len)
            ++in->pos;
    } else if (k == KEY_CTRL('D')) {
        if (in->pos < in->len)
            input_erase(in, in->pos);
    } else if (k == KEY_CTRL('A')) {
        in->pos = 0;
    } else if (k == KEY_CTRL('E')) {
        in->pos = in->len;
    } else {
        return EVENT_NOT_HANDLED;
    }
    input_fix_scroll(in);
    view_invalidate(v);
    return EVENT_HANDLED;
}

static const TViewOps input_ops = { input_draw, input_handle };

void input_init(TInputLine *in, u8 x, u8 y, u8 w, char *buf, u8 max)
{
    view_init(&in->v, x, y, w, 1, &input_ops, VF_FOCUSABLE);
    in->buf = buf;
    in->max = max;
    in->len = in->pos = in->scroll = 0;
    buf[0] = '\0';
}

void input_set_text(TInputLine *in, const char *s)
{
    in->len = (u8)strlen(s);
    if (in->len > in->max)
        in->len = in->max;
    memcpy(in->buf, s, in->len);
    in->buf[in->len] = '\0';
    in->pos = in->len;
    in->scroll = 0;
    input_fix_scroll(in);
    view_invalidate(&in->v);
}
