/* app.c -- bureau, boucle principale, dialogues modaux. */

#include "app.h"
#include "screen.h"
#include "keyboard.h"
#include "tui_mouse.h"

void (*app_idle_hook)(void);
u16 app_entropy;

static TGroup desktop;
static TMenuBar *menubar;
static const TMenuHooks *menu_hooks_ptr;   /* voir TMenuHooks (menu.h) */
static const char *status_text;
static u8 status_dirty;
static u8 running;
static u8 (*cmd_handler)(TEvent *ev);

static void desktop_draw(TView *v)
{
    TGroup *g = (TGroup *)v;
    if (v->flags & VF_DIRTY) {
        u8 ax, ay;
        view_abs(v, &ax, &ay);
        scr_fill(ax, ay, v->w, v->h, ' ', ATTR_NORMAL);
        group_draw_children(g, 1);
    } else {
        group_draw_children(g, 0);
    }
}

static const TViewOps desktop_ops = { desktop_draw, group_handle };

void app_init(void)
{
    scr_init();
    mouse_init();
    group_init(&desktop, 0, 1, scr_cols, TUI_DESK_ROWS, &desktop_ops, 0);
    menubar = 0;
    menu_hooks_ptr = 0;
    status_text = 0;
    status_dirty = 0;
    running = 0;
    cmd_handler = 0;
}

void app_done(void)
{
    mouse_done();
    scr_done();
}

TGroup *app_desktop(void)         { return &desktop; }
u8 app_insert(TView *v)           { return group_insert(&desktop, v); }
void app_set_handler(u8 (*h)(TEvent *ev)) { cmd_handler = h; }
void app_quit(void)               { running = 0; }

void app_set_menubar(TMenuBar *mb)
{
    menubar = mb;
    if (mb)
        view_invalidate(&mb->v);
}

void app_set_menu_hooks(const TMenuHooks *h)
{
    menu_hooks_ptr = h;
}

void app_set_status(const char *text)
{
    status_text = text;
    status_dirty = 1;
}

void app_redraw(void)
{
    if (menubar)
        view_draw(&menubar->v);
    view_draw(&desktop.v);
    if (status_dirty) {
        scr_putsw(0, TUI_SCR_ROWS - 1, status_text ? status_text : "", scr_cols,
                  ATTR_INVERSE);
        status_dirty = 0;
    }
    scr_flush();
}

void app_repaint_all(void)
{
    if (menubar)
        view_invalidate(&menubar->v);
    view_invalidate(&desktop.v);
    status_dirty = 1;
}

void app_wait_event(TEvent *ev)
{
    for (;;) {
        event_poll(ev);
        if (ev->type != EV_NONE)
            return;
        ++app_entropy;
        if (app_idle_hook)
            app_idle_hook();
    }
}

/* Ordre : raccourci de menu -> vues (bureau) -> Echap ouvre le menu ->
 * commande restante au handler applicatif, puis CM_QUIT. */
static void dispatch(TEvent *ev)
{
    u8 cmd;
    if (ev->type == EV_KEY && menubar) {
        cmd = menu_hooks_ptr->find_key(menubar, ev->key);
        if (cmd != CM_NONE) {
            ev->type = EV_COMMAND;
            ev->cmd = cmd;
        }
    }
    for (;;) {
#if TUI_MOUSE
        if (ev->type == EV_MOUSE_DOWN && menubar && ev->mouse_y == 0) {
            cmd = menu_hooks_ptr->title_at(menubar, ev->mouse_x);
            if (cmd == 0xFF)
                return;
            cmd = menu_hooks_ptr->open(menubar, cmd);
            if (cmd == CM_NONE)
                return;
            ev->type = EV_COMMAND;
            ev->cmd = cmd;
            continue;
        }
#endif
        if (desktop.v.ops->handle(&desktop.v, ev) == EVENT_HANDLED)
            return;
        if (ev->type == EV_KEY && ev->key == KEY_ESC && menubar) {
            cmd = menu_hooks_ptr->run(menubar);
            if (cmd == CM_NONE)
                return;
            ev->type = EV_COMMAND;
            ev->cmd = cmd;
            continue;
        }
        break;
    }
    if (ev->type == EV_COMMAND) {
        if (cmd_handler && cmd_handler(ev) == EVENT_HANDLED)
            return;
        if (ev->cmd == CM_QUIT)
            running = 0;
    }
}

void app_step(void)
{
    TEvent ev;
    event_poll(&ev);
    if (ev.type != EV_NONE) {
        dispatch(&ev);
    } else {
        ++app_entropy;
        if (app_idle_hook)
            app_idle_hook();
    }
    app_redraw();
}

void app_run(void)
{
    running = 1;
    app_redraw();
    while (running)
        app_step();
}

void app_modal_open(TWindow *dlg)
{
    dlg->result = 0;
    dlg->wflags |= WF_MODAL;
    group_insert(&desktop, &dlg->g.v);
    group_focus_view(&desktop, &dlg->g.v);
    app_redraw();
}

void app_modal_close(TWindow *dlg)
{
    group_remove(&desktop, &dlg->g.v);
    view_invalidate(&desktop.v);     /* pas de save-under : le bureau se redessine */
    app_redraw();
}

u8 app_exec(TWindow *dlg)
{
    TEvent ev;
    app_modal_open(dlg);
    while (dlg->result == 0) {
        app_wait_event(&ev);
        if (dlg->g.v.ops->handle(&dlg->g.v, &ev) == EVENT_NOT_HANDLED &&
            ev.type == EV_COMMAND && cmd_handler)
            cmd_handler(&ev);
        app_redraw();
    }
    app_modal_close(dlg);
    return dlg->result;
}
