/* keyboard.c -- clavier Apple II par scrutation. */

#include "keyboard.h"

#ifdef __CC65__

#define KBD      (*(volatile u8 *)0xC000)
#define KBDSTRB  (*(volatile u8 *)0xC010)
#define BTN0     (*(volatile u8 *)0xC061)
#define BTN1     (*(volatile u8 *)0xC062)

u8 kbd_poll(void)
{
    u8 c;
    if ((KBD & 0x80) == 0)
        return 0;
    c = KBD & 0x7F;
    KBDSTRB = 0;                   /* ecriture : la lecture jetee (void)KBDSTRB n'a pas
                                      acquitte le strobe (touche repetee) */
    return c;
}

u8 kbd_modifiers(void)
{
    return (u8)(((BTN0 & 0x80) ? MOD_OPEN_APPLE : 0) |
                ((BTN1 & 0x80) ? MOD_CLOSED_APPLE : 0));
}

#else /* hote : file de touches injectees */

#include <stdio.h>
#include <stdlib.h>

static u8 q[512];
static unsigned qh, qt;
static unsigned long idle;

void tui_host_push_key(u8 k) { q[qt++ % sizeof q] = k; }
void tui_host_clear_keys(void) { qh = qt = 0; }

void tui_host_push_keys(const char *s)
{
    while (*s)
        tui_host_push_key((u8)*s++);
}

u8 kbd_poll(void)
{
    if (qh != qt) {
        idle = 0;
        return q[qh++ % sizeof q];
    }
    /* Une attente de touche sans plus rien a lire : boucle infinie. */
    if (++idle > 100000UL) {
        fprintf(stderr, "kbd_poll: file de touches vide, boucle infinie evitee\n");
        exit(2);
    }
    return 0;
}

u8 tui_host_mods;
u8 kbd_modifiers(void) { return tui_host_mods; }

#endif
