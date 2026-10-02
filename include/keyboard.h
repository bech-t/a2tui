/* keyboard.h -- lecture clavier bas niveau ($C000/$C010). */
#ifndef TUI_KEYBOARD_H
#define TUI_KEYBOARD_H

#include "tui_config.h"

#define KEY_LEFT   0x08
#define KEY_TAB    0x09
#define KEY_DOWN   0x0A
#define KEY_UP     0x0B
#define KEY_ENTER  0x0D
#define KEY_RIGHT  0x15
#define KEY_ESC    0x1B
#define KEY_DEL    0x7F

#define KEY_CTRL(c) ((c) & 0x1F)   /* KEY_CTRL('S') = 0x13 */

#define MOD_OPEN_APPLE   1
#define MOD_CLOSED_APPLE 2

/* Renvoie le code ASCII 7 bits d'une touche en attente (et efface le
 * strobe), ou 0 s'il n'y en a pas. Ne bloque jamais. */
u8 kbd_poll(void);
/* Etat des touches Pomme ouverte/fermee (boutons 0/1 de la manette). */
u8 kbd_modifiers(void);

#ifdef TUI_HOST
extern u8 tui_host_mods;            /* valeur renvoyee par kbd_modifiers() */
void tui_host_push_key(u8 k);
void tui_host_clear_keys(void);
void tui_host_push_keys(const char *s);
#endif

#endif
