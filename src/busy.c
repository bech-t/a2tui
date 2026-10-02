/* busy.c -- voir busy.h.
 *
 * Fenetre/label STATIQUES ET DEDIEES (pas dlg_win/dlg_item de dialog.h) :
 * link_request() peut etre appele alors qu'un dialogue plein ecran est deja
 * construit (dialog_build()) ou actif (dialog_run()/app_exec()) -- voir
 * explorer.c:xp_refresh(), qui appelle link_request() entre les deux.
 * Reutiliser dlg_win depuis busy_show() ecraserait ce dialogue. Une fenetre
 * separee, simple enfant de plus sur le bureau, coexiste sans toucher a
 * l'etat de quoi que ce soit d'autre. */
#include <string.h>
#include "window.h"
#include "widgets.h"
#include "app.h"
#include "busy.h"

#define BUSY_BUF 36

static const char spin_frames[] = "-+*x";
#define SPIN_COUNT ((u8)(sizeof spin_frames - 1))

#define BUSY_BAR_W 20

static TWindow busy_win;
static TLabel busy_lbl;
static TProgress busy_bar;
static char busy_line[BUSY_BUF];
static u8 busy_len;     /* longueur de busy_line avant l'espace+tourniquet */
static u8 spin_idx;
static u8 spin_started; /* faux tant que busy_spin() n'a pas ete appele */
static u8 busy_active;
static u8 bar_active;   /* barre de progression affichee (busy_set_progress()) */
static u16 bar_value, bar_max;

/* (Re)construit busy_win/busy_lbl/[busy_bar] a partir de busy_line/busy_len
 * et bar_active/bar_value/bar_max : busy_show(), busy_set_text() et le
 * premier busy_set_progress() (qui fait grandir la boite) passent tous par
 * ici pour rester coherents. */
static void busy_rebuild(void)
{
    u8 inner_w = (u8)(busy_len + 2);
    u8 box_w = inner_w;
    u8 box_h = 3;

    if (bar_active) {
        if ((u8)(BUSY_BAR_W + 2) > box_w)
            box_w = BUSY_BAR_W + 2;
        box_h = 4;
    }

    if (busy_active)
        group_remove(app_desktop(), &busy_win.g.v);

    window_init(&busy_win, 0, 0, (u8)(box_w + 4), box_h, 0, 0);
    window_center(&busy_win);
    label_init(&busy_lbl, 2, 1, inner_w, busy_line);
    window_add(&busy_win, &busy_lbl.v);
    if (bar_active) {
        progress_init(&busy_bar, 2, 2, BUSY_BAR_W, bar_max);
        progress_set(&busy_bar, bar_value);
        window_add(&busy_win, &busy_bar.v);
    }
    app_insert(&busy_win.g.v);
    busy_active = 1;
    view_invalidate(&app_desktop()->v);
    app_redraw();
}

void busy_show(const char *text)
{
    u8 len = (u8)strlen(text);

    if (len > BUSY_BUF - 3)
        len = BUSY_BUF - 3;   /* laisse la place a " " + 1 caractere + NUL */
    memcpy(busy_line, text, len);
    busy_line[len] = ' ';
    busy_line[len + 1] = ' ';   /* pas de tourniquet tant que busy_spin() n'a
                                 * pas ete appele -- sinon un "|" figé, sans
                                 * animation, si l'appelant ne fait pas de
                                 * tick (cas d'une E/S sans callback). */
    busy_line[len + 2] = '\0';
    busy_len = len;
    spin_idx = (u8)(SPIN_COUNT - 1);   /* le premier busy_spin() affiche le cadre 0 */
    spin_started = 0;
    bar_active = 0;

    busy_rebuild();
}

void busy_spin(void)
{
    if (!busy_active)
        return;
    spin_started = 1;
    spin_idx = (u8)((spin_idx + 1) % SPIN_COUNT);
    busy_line[busy_len + 1] = spin_frames[spin_idx];
    label_set(&busy_lbl, busy_line);
    app_redraw();
}

void busy_set_text(const char *text)
{
    u8 len;

    if (!busy_active)
        return;
    len = (u8)strlen(text);
    if (len > BUSY_BUF - 3)
        len = BUSY_BUF - 3;
    memcpy(busy_line, text, len);
    busy_line[len] = ' ';
    busy_line[len + 1] = spin_started ? spin_frames[spin_idx] : ' ';
    busy_line[len + 2] = '\0';
    busy_len = len;

    /* La boite est redimensionnee sur ce nouveau texte (busy_rebuild()),
     * sans perdre l'etat du tourniquet (gere au-dessus) ni celui d'une
     * barre de progression deja affichee (bar_active/bar_value/bar_max,
     * inchanges ici). */
    busy_rebuild();
}

/* Affiche (au premier appel) ou met a jour une barre de progression sous le
 * texte -- la boite grandit une fois pour l'accueillir (busy_rebuild()),
 * puis les appels suivants a max inchange ne font qu'une mise a jour legere
 * (progress_set() + app_redraw(), pas de redimensionnement ni de
 * scintillement). No-op avant busy_show()/apres busy_hide(). */
void busy_set_progress(u16 value, u16 max)
{
    if (!busy_active)
        return;
    if (bar_active && max == bar_max) {
        bar_value = value > max ? max : value;
        progress_set(&busy_bar, bar_value);
        app_redraw();
        return;
    }
    bar_active = 1;
    bar_max = max;
    bar_value = value > max ? max : value;
    busy_rebuild();
}

void busy_hide(void)
{
    if (!busy_active)
        return;
    busy_active = 0;
    bar_active = 0;
    group_remove(app_desktop(), &busy_win.g.v);
    view_invalidate(&app_desktop()->v);
    app_redraw();
}
