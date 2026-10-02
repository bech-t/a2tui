/* mouse.c -- souris : driver standard cc65 ; file d'evenements simulee sur PC. */

#include "tui_mouse.h"
#include "event.h"
#include "screen.h"

#if TUI_MOUSE && defined(__CC65__)

#include <mouse.h>

/* Le curseur est dessine par screen.c : les callbacks du driver ne font rien.
 * Ils peuvent etre appeles sous interruption, d'ou l'absence de locales. */
static void cb_nop(void) {}
static void __fastcall__ cb_move(int v) { (void)v; }
static const struct mouse_callbacks callbacks = {
    cb_nop, cb_nop, cb_nop, cb_nop, cb_move, cb_move
};

static u8 present, last_x, last_y, last_b;

u8 mouse_init(void)
{
    struct mouse_box box;
    struct mouse_info mi;
    present = 0;
    if (mouse_install(&callbacks, (void *)mouse_static_stddrv) != MOUSE_ERR_OK)
        return 0;
    /* 8 unites par cellule : position -> cellule par un simple decalage. */
    box.minx = 0;
    box.miny = 0;
    box.maxx = (int)scr_cols * 8 - 1;
    box.maxy = (int)TUI_SCR_ROWS * 8 - 1;
    mouse_setbox(&box);
    mouse_move((int)scr_cols * 4, TUI_SCR_ROWS * 4);
    mouse_info(&mi);
    last_x = (u8)(mi.pos.x >> 3);
    last_y = (u8)(mi.pos.y >> 3);
    last_b = 0;
    present = 1;
    scr_mouse_move(last_x, last_y);
    return 1;
}

void mouse_done(void)
{
    if (present) {
        scr_mouse_show(0);
        mouse_uninstall();
        present = 0;
    }
}

u8 mouse_present(void) { return present; }

u8 mouse_poll(TEvent *ev)
{
    struct mouse_info mi;
    u8 cx, cy, b;
    if (!present)
        return 0;
    mouse_info(&mi);
    cx = (u8)(mi.pos.x >> 3);
    cy = (u8)(mi.pos.y >> 3);
    if (cx >= scr_cols) cx = (u8)(scr_cols - 1);
    if (cy >= TUI_SCR_ROWS) cy = TUI_SCR_ROWS - 1;
    b = (mi.buttons & MOUSE_BTN_LEFT) ? 1 : 0;
    ev->mouse_x = cx;
    ev->mouse_y = cy;
    ev->buttons = b;
    if (cx != last_x || cy != last_y) {
        last_x = cx;
        last_y = cy;
        scr_mouse_move(cx, cy);
        if (b == last_b) {
            ev->type = EV_MOUSE_MOVE;
            return 1;
        }
    }
    if (b != last_b) {
        last_b = b;
        ev->type = b ? EV_MOUSE_DOWN : EV_MOUSE_UP;
        return 1;
    }
    return 0;
}

#elif defined(TUI_HOST) && TUI_MOUSE

static struct { u8 type, x, y; } q[32];
static unsigned qh, qt;
static u8 present, held;

void tui_host_push_mouse(u8 type, u8 x, u8 y)
{
    q[qt++ % 32].type = type;
    q[(qt - 1) % 32].x = x;
    q[(qt - 1) % 32].y = y;
}

u8 mouse_init(void)
{
    present = 1;
    held = 0;
    qh = qt = 0;
    scr_mouse_move(scr_cols / 2, 12);
    return 1;
}
void mouse_done(void) { scr_mouse_show(0); present = 0; }
u8 mouse_present(void) { return present; }

u8 mouse_poll(TEvent *ev)
{
    if (!present || qh == qt)
        return 0;
    ev->type = q[qh % 32].type;
    ev->mouse_x = q[qh % 32].x;
    ev->mouse_y = q[qh % 32].y;
    if (ev->type == EV_MOUSE_DOWN) held = 1;
    if (ev->type == EV_MOUSE_UP) held = 0;
    ev->buttons = held;
    ++qh;
    scr_mouse_move(ev->mouse_x, ev->mouse_y);
    return 1;
}

#else /* TUI_MOUSE = 0 */

u8 mouse_init(void) { return 0; }
void mouse_done(void) {}
u8 mouse_present(void) { return 0; }
u8 mouse_poll(TEvent *ev) { (void)ev; return 0; }

#endif
