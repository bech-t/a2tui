/* event.c -- une source d'evenements normalisee pour la couche Views. */

#include "event.h"
#include "keyboard.h"
#include "tui_mouse.h"

void event_poll(TEvent *ev)
{
    u8 k = kbd_poll();
    ev->type = EV_NONE;
    ev->key = ev->cmd = ev->mouse_x = ev->mouse_y = ev->buttons = ev->mods = 0;
    if (k) {
        ev->type = EV_KEY;
        ev->key = k;
        ev->mods = kbd_modifiers();
        return;
    }
    mouse_poll(ev);
}
