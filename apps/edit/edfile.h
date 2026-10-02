/* edfile.h -- fichiers texte et repertoire courant (ProDOS via cc65, POSIX sur PC). */
#ifndef EDFILE_H
#define EDFILE_H

#include "tui_config.h"

#define ED_OK       0
#define ED_ERR_OPEN 1     /* ouverture/creation impossible */
#define ED_ERR_BIG  2     /* fichier plus grand que le tampon */
#define ED_ERR_IO   3     /* erreur de lecture/ecriture (disque plein...) */

/* Dernier code d'erreur du systeme (ProDOS MLI sur Apple II), 0 si inconnu. */
extern u8 ed_os_error;

/* Charge name dans le tampon de texte (CR/LF/CRLF -> CR, bit 7 efface). Le
 * tampon n'est modifie qu'apres une ouverture reussie. */
u8 ed_load_file(const char *name);
u8 ed_save_file(const char *name);

#endif
