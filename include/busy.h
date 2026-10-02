/* busy.h -- indicateur d'attente (tourniquet ASCII : | / - \) pour une
 * operation bloquante (E/S serie ou disque) qui n'a sinon aucun retour
 * visuel pendant sa duree. Contrairement aux dialogues de dialog.h, pas
 * d'app_exec() : on est typiquement DANS une boucle de blocage (attente
 * liaison serie, lecture disque...) quand on s'en sert, donc chaque
 * fonction dessine et flush immediatement plutot que d'attendre un tour de
 * la boucle d'evenements normale. Usage :
 *
 *   busy_show("Loading...");
 *   while (toujours_en_attente) {
 *       ...un tour de la boucle bloquante...
 *       busy_spin();
 *   }
 *   busy_hide();
 *
 * Fenetre/label dedies (PAS dlg_win/dlg_item de dialog.h) : safe a appeler
 * meme pendant qu'un dialogue plein ecran est deja construit (dialog_build())
 * ou en cours d'execution (dialog_run()/app_exec()), par exemple
 * link_request() appele depuis explorer.c pendant que l'explorateur
 * construit ou affiche son propre dialogue. Simple enfant de plus sur le
 * bureau, insere puis retire sans toucher a l'etat de personne d'autre. */
#ifndef TUI_BUSY_H
#define TUI_BUSY_H

#include "tui_config.h"

/* Affiche une petite boite centree "text" (text tronque si besoin), SANS
 * tourniquet -- il n'apparait qu'au premier busy_spin(). Reste propre (pas
 * de caractere fige) pour un appelant qui n'a pas de point d'appel
 * periodique pendant son E/S (ex. une simple lecture de fichier). */
void busy_show(const char *text);
/* Avance le tourniquet d'un cran (| / - \, cadre 0 au premier appel) et
 * redessine. A appeler a intervalles reguliers pendant l'attente. */
void busy_spin(void);
/* Remplace le texte (ex. une progression "Copying 3/12") et redimensionne/
 * recentre la boite en consequence, sans toucher au cadre courant du
 * tourniquet ni a l'etat "pas encore anime" tant que busy_spin() n'a pas
 * ete appele. No-op avant busy_show()/apres busy_hide(). */
void busy_set_text(const char *text);
/* Affiche (au premier appel) ou met a jour une barre de progression sous le
 * texte (TProgress, widgets.h) -- value borne a max. La boite grandit une
 * fois pour l'accueillir ; les mises a jour suivantes a max inchange sont
 * legeres (pas de redimensionnement). No-op avant busy_show()/apres
 * busy_hide(). */
void busy_set_progress(u16 value, u16 max);
/* Referme la boite (le dessous se redessine, comme app_exec() a la
 * fermeture d'un dialogue). */
void busy_hide(void);

#endif
