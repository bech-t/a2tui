/* filesel.h -- selecteur de fichier / de dossier (ProDOS sur Apple II via
 * cc65, POSIX sur PC). Aucun texte visible dans la bibliotheque : tous les
 * libelles viennent de l'appelant (TFileSelText).
 *
 * Utilise le jeu de widgets partage de dialog.h : ne pas l'appeler pendant
 * qu'un autre dialogue (dialog_run) est ouvert. Le repertoire courant du
 * programme n'est jamais modifie. */
#ifndef TUI_FILESEL_H
#define TUI_FILESEL_H

#include "dialog.h"

#define FS_OPEN   0   /* choisir un fichier existant */
#define FS_SAVE   1   /* choisir ou saisir un nom ; confirmation avant d'ecraser */
#define FS_FOLDER 2   /* choisir un dossier ; la saisie sert a en creer un */

/* Taille du tampon de chemin de l'appelant (NUL compris) et nombre d'entrees
 * de dossier affichables. */
#define FS_PATH_LEN 65
#define FS_DIR_MAX  32

typedef struct {
    const char *name;     /* etiquette de la saisie ("Name:") */
    const char *ok;       /* bouton de validation ("Open", "Save", "Select") */
    const char *cancel;   /* bouton d'annulation */
    const char *newdir;   /* bouton de creation de dossier (FS_FOLDER) */
    const char *exists;   /* confirmation d'ecrasement (FS_SAVE), '\n' autorise */
    const char *error;    /* echec de lecture / de creation, '\n' autorise */
} TFileSelText;

/* title : titre de la fenetre. path : entree/sortie, tampon de FS_PATH_LEN octets.
 * En entree : chemin complet ou nom relatif d'un fichier propose (FS_OPEN,
 * FS_SAVE) ou dossier de depart (FS_FOLDER) ; "" = repertoire courant.
 * En sortie (CM_OK) : chemin complet du fichier ou du dossier choisi ; le
 * contenu de path est indefini apres CM_CANCEL.
 * Renvoie CM_OK ou CM_CANCEL. */
u8 filesel(u8 mode, const char *title, const TFileSelText *t, char *path);

#endif
