/* lang.h -- moteur i18n minimal : charge un fichier texte "cle=valeur" (une
 * entree par ligne, '#' ou ';' en tete de ligne = commentaire, "\n" deplie
 * en un vrai saut de ligne dans la valeur -- seule facon d'exprimer un
 * message multi-lignes dans ce format une-entree-par-ligne) et remplit une
 * table de chaines fournie par l'appli. Le moteur ne connait aucune cle ni
 * aucun format de disque : c'est l'appli qui fournit la liste des cles
 * attendues et les tampons de destination (deja remplis avec le texte
 * source par defaut). Choisir quel fichier charger -- typiquement en
 * listant un sous-dossier LANG/ ProDOS -- est hors du champ de ce module,
 * cote appli (parcours de repertoire deja specifique a chacune). */
#ifndef A2TUI_LANG_H
#define A2TUI_LANG_H

#include "tui_config.h"

typedef struct {
    const char *key;     /* cle attendue, ex. "MSG_QUIT" */
    char       *buf;     /* tampon de destination (deja rempli par defaut) */
    u8          buf_len; /* taille du tampon, \0 compris */
} LangEntry;

/* Charge path et remplit table[0..count) pour chaque cle trouvee dans le
 * fichier. Les cles absentes du fichier gardent leur valeur : l'appli doit
 * donc preremplir les tampons avec le texte par defaut avant l'appel.
 * Renvoie 0 si le fichier a pu etre ouvert, 1 sinon (table inchangee). */
u8 lang_load(const char *path, LangEntry *table, u8 count);

#endif
