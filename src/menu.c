/* menu.c -- TMenuBar / TMenu. Le menu deroulant se dessine directement dans
 * le tampon virtuel apres sauvegarde du rectangle recouvert (le tampon EST
 * l'image de l'ecran : pas besoin de redessiner les vues dessous). */

#include <string.h>
#include "menu.h"
#include "screen.h"
#include "keyboard.h"
#include "app.h"

static u8 save_buf[TUI_MENU_SAVE];

static u8 is_sep(const TMenuItem *it)
{
    return it->text[0] == '-' && it->text[1] == '\0';
}

static u8 title_x(const TMenuBar *mb, u8 m)
{
    u8 i, x = 1;
    for (i = 0; i < m; ++i)
        x += (u8)strlen(mb->menus[i].title) + 3;
    return x;
}

static void bar_draw(TMenuBar *mb, u8 active)
{
    u8 i, x;
    scr_fill(0, 0, scr_cols, 1, ' ', ATTR_INVERSE);
    for (i = 0; i < mb->count; ++i) {
        x = title_x(mb, i);
        if (i == active)
            scr_fill(x, 0, (u8)strlen(mb->menus[i].title) + 2, 1, ' ', ATTR_NORMAL);
        scr_puts(x + 1, 0, mb->menus[i].title, i == active ? ATTR_NORMAL : ATTR_INVERSE);
    }
}

static void bar_view_draw(TView *v)
{
    bar_draw((TMenuBar *)v, 0xFF);
    v->flags &= ~VF_DIRTY;
}

static u8 bar_view_handle(TView *v, TEvent *ev)
{
    (void)v; (void)ev;
    return EVENT_NOT_HANDLED;
}

static const TViewOps bar_ops = { bar_view_draw, bar_view_handle };

/* Enregistree aupres de app.c (app_set_menu_hooks) sans que app.c ait a
 * nommer une seule fonction d'ici -- c'est ce qui permet a une appli qui
 * n'utilise jamais de barre de menu de ne pas lier ce fichier du tout. */
const TMenuHooks menu_hooks = {
    menubar_find_key, menubar_run, menubar_open, menubar_title_at
};

void menubar_init(TMenuBar *mb, const TMenu *menus, u8 count)
{
    view_init(&mb->v, 0, 0, scr_cols, 1, &bar_ops, 0);
    mb->menus = menus;
    mb->count = count;
    app_set_menu_hooks(&menu_hooks);
}

u8 menubar_find_key(const TMenuBar *mb, u8 key)
{
    u8 m, i;
    for (m = 0; m < mb->count; ++m)
        for (i = 0; i < mb->menus[m].count; ++i)
            if (mb->menus[m].items[i].key == key && key != 0)
                return mb->menus[m].items[i].cmd;
    return CM_NONE;
}

/* Rectangle du menu deroulant m. */
static void geom(const TMenuBar *mb, u8 m, u8 *x, u8 *w, u8 *h)
{
    const TMenu *menu = &mb->menus[m];
    u8 i, len, maxl = 0, hint = 0;
    for (i = 0; i < menu->count; ++i) {
        len = (u8)strlen(menu->items[i].text);
        if (len > maxl)
            maxl = len;
        if (menu->items[i].key)
            hint = 4;
    }
    *w = (u8)(maxl + 4 + hint);
    *h = (u8)(menu->count + 2);
    *x = title_x(mb, m);
    if (*x + *w > scr_cols)
        *x = (u8)(scr_cols - *w);
}

static void drop_draw(const TMenuBar *mb, u8 m, u8 sel)
{
    const TMenu *menu = &mb->menus[m];
    u8 x, w, h, i, ax;
    const TMenuItem *it;
    geom(mb, m, &x, &w, &h);
    scr_box(x, 1, w, h);
    for (i = 0; i < menu->count; ++i) {
        it = &menu->items[i];
        if (is_sep(it)) {
            for (ax = 1; ax + 1 < w; ++ax)
                scr_putc(x + ax, (u8)(2 + i), G_HSEP, ATTR_NORMAL);
            continue;
        }
        if (i == sel)
            scr_fill(x + 1, (u8)(2 + i), (u8)(w - 2), 1, ' ', ATTR_INVERSE);
        scr_puts(x + 2, (u8)(2 + i), it->text, i == sel ? ATTR_INVERSE : ATTR_NORMAL);
        if (it->key) {
            scr_putc(x + w - 5, (u8)(2 + i), '^', i == sel ? ATTR_INVERSE : ATTR_NORMAL);
            scr_putc(x + w - 4, (u8)(2 + i), (char)(it->key + '@'),
                     i == sel ? ATTR_INVERSE : ATTR_NORMAL);
        }
    }
}

static u8 step_item(const TMenu *menu, u8 sel, u8 forward)
{
    u8 n;
    for (n = 0; n < menu->count; ++n) {
        sel = forward ? (u8)((sel + 1) % menu->count)
                      : (u8)(sel ? sel - 1 : menu->count - 1);
        if (!is_sep(&menu->items[sel]))
            return sel;
    }
    return sel;
}

u8 menubar_title_at(const TMenuBar *mb, u8 x)
{
    u8 i, t;
    for (i = 0; i < mb->count; ++i) {
        t = title_x(mb, i);
        if (x >= t && x < t + (u8)strlen(mb->menus[i].title) + 2)
            return i;
    }
    return 0xFF;
}

u8 menubar_run(TMenuBar *mb)
{
    return menubar_open(mb, 0);
}

u8 menubar_open(TMenuBar *mb, u8 m)
{
    u8 sel, x, w, h, cmd = CM_NONE, open;
#if TUI_MOUSE
    u8 t;
#endif
    TEvent ev;

    sel = step_item(&mb->menus[m], mb->menus[m].count - 1, 1);
    for (;;) {
        geom(mb, m, &x, &w, &h);
        open = (u16)w * h <= TUI_MENU_SAVE;
        if (open)
            scr_save(x, 1, w, h, save_buf);
        bar_draw(mb, m);
        if (open)
            drop_draw(mb, m, sel);
        scr_flush();

        app_wait_event(&ev);

        if (open)
            scr_restore(x, 1, w, h, save_buf);
#if TUI_MOUSE
        if (ev.type == EV_MOUSE_DOWN) {
            if (ev.mouse_y == 0) {
                t = menubar_title_at(mb, ev.mouse_x);
                if (t == 0xFF)
                    break;
                m = t;
                sel = step_item(&mb->menus[m], mb->menus[m].count - 1, 1);
            } else if (open && ev.mouse_x > x && ev.mouse_x < x + w - 1 &&
                       ev.mouse_y >= 2 && ev.mouse_y < 2 + mb->menus[m].count) {
                t = (u8)(ev.mouse_y - 2);
                if (is_sep(&mb->menus[m].items[t]))
                    continue;
                cmd = mb->menus[m].items[t].cmd;
                break;
            } else {
                break;                      /* clic ailleurs : referme */
            }
            continue;
        }
        if (ev.type == EV_MOUSE_MOVE) {     /* survol : suit la souris */
            if (open && ev.mouse_x > x && ev.mouse_x < x + w - 1 &&
                ev.mouse_y >= 2 && ev.mouse_y < 2 + mb->menus[m].count &&
                !is_sep(&mb->menus[m].items[ev.mouse_y - 2]))
                sel = (u8)(ev.mouse_y - 2);
            continue;
        }
#endif
        if (ev.type != EV_KEY)
            continue;
        if (ev.key == KEY_ESC) {
            break;
        } else if (ev.key == KEY_ENTER) {
            cmd = mb->menus[m].items[sel].cmd;
            break;
        } else if (ev.key == KEY_LEFT || ev.key == KEY_RIGHT) {
            m = ev.key == KEY_RIGHT ? (u8)((m + 1) % mb->count)
                                    : (u8)(m ? m - 1 : mb->count - 1);
            sel = step_item(&mb->menus[m], mb->menus[m].count - 1, 1);
        } else if (ev.key == KEY_DOWN) {
            sel = step_item(&mb->menus[m], sel, 1);
        } else if (ev.key == KEY_UP) {
            sel = step_item(&mb->menus[m], sel, 0);
        } else if (ev.key > ' ') {           /* initiale d'un article : le declenche */
            u8 k = ev.key, i;
            const TMenu *mn = &mb->menus[m];
            if (k >= 'a' && k <= 'z')
                k -= 32;
            for (i = 0; i < mn->count; ++i) {
                u8 c = (u8)mn->items[i].text[0];
                if (c >= 'a' && c <= 'z')
                    c -= 32;
                if (c == k && !is_sep(&mn->items[i])) {
                    cmd = mn->items[i].cmd;
                    goto done;
                }
            }
        }
    }
done:
    view_invalidate(&mb->v);
    return cmd;
}
