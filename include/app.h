/* app.h -- application : bureau, barre de menu, barre d'etat, boucle
 * principale poll -> dispatch -> redraw -> flush. */
#ifndef TUI_APP_H
#define TUI_APP_H

#include "tview.h"
#include "menu.h"
#include "window.h"

/* Initialise l'ecran et le bureau. A appeler en premier. */
void app_init(void);
/* Restaure un ecran texte 40 colonnes propre. */
void app_done(void);

/* Bureau : les fenetres non modales y sont inserees (la derniere inseree
 * a le focus). Coordonnees des fenetres relatives au bureau (ligne 1). */
TGroup *app_desktop(void);
u8   app_insert(TView *v);

void app_set_menubar(TMenuBar *mb);
/* Appele par menubar_init() -- jamais directement : voir TMenuHooks (menu.h). */
void app_set_menu_hooks(const TMenuHooks *h);
void app_set_status(const char *text);

/* Recoit toute commande que ni les vues ni un dialogue n'ont consommee.
 * Renvoie EVENT_HANDLED pour l'arreter (sinon CM_QUIT termine app_run). */
void app_set_handler(u8 (*handler)(TEvent *ev));

void app_run(void);
void app_step(void);       /* une iteration de la boucle */
void app_quit(void);

/* Dialogue modal : l'insere, tourne jusqu'a sa fermeture, le retire et
 * renvoie la commande de fermeture (CM_OK, CM_CANCEL...). */
u8   app_exec(TWindow *dlg);

/* app_exec() en deux temps, pour un appelant qui doit tenir sa propre boucle
 * (ex. une boucle de reception serie qui ne tolere aucun retard) : ouvre le
 * dialogue modal, puis le referme. Entre les deux, l'appelant lit les
 * evenements (event_poll()), les donne a dlg->g.v.ops->handle() et appelle
 * app_redraw() ; le dialogue est termine quand dlg->result != 0. */
void app_modal_open(TWindow *dlg);
void app_modal_close(TWindow *dlg);

/* Redessine ce qui est sale puis flush. */
void app_redraw(void);
/* Tout redessiner (le flush n'ecrira que ce qui differe). */
void app_repaint_all(void);
/* Attend un evenement (bloquant), en appelant app_idle_hook. */
void app_wait_event(TEvent *ev);

extern void (*app_idle_hook)(void);   /* travail de fond pendant l'attente */
extern u16 app_entropy;               /* compteur incremente pendant l'attente */

#endif
