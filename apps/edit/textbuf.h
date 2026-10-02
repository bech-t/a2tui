/* textbuf.h -- tampon de texte en gap buffer, en RAM principale.
 *
 * Le texte est interne en octets 7 bits, fin de ligne = CR ($0D), comme dans
 * un fichier texte ProDOS. */
#ifndef TEXTBUF_H
#define TEXTBUF_H

#include "tui_config.h"

#define TB_NONE  0xFFFFu

/* Capacite du tampon (octets), fixee par tb_init(). */
extern u16 tb_cap;

/* A appeler une fois au demarrage. Avec de la RAM auxiliaire (auxmem.h) le
 * tampon y est place ; sinon il occupe toute la RAM principale libre entre la
 * fin du programme (BSS) et la pile (sur PC : un tableau fixe). */
void tb_init(void);

void tb_clear(void);
u16  tb_len(void);
u16  tb_free(void);
u8   tb_at(u16 i);                              /* octet a l'offset i (i < len) */
u8   tb_insert(u16 pos, const u8 *s, u16 n);    /* 0 si plein */
void tb_delete(u16 pos, u16 n);
void tb_copy(u16 pos, u16 n, u8 *dst);
/* Octets contigus a partir de pos (< len) : pointeur, et *n = nombre disponible.
 * Valable jusqu'au prochain appel de tb_*. */
const u8 *tb_ptr(u16 pos, u16 *n);
u16  tb_line_start(u16 pos);                    /* debut de la ligne contenant pos */
u16  tb_line_end(u16 pos);                      /* offset du CR de fin de ligne, ou len */
u16  tb_count_cr(u16 from, u16 to);             /* nombre de CR dans [from, to) */
/* Recherche vers l'avant a partir de from ; TB_NONE si absent. */
u16  tb_find(u16 from, const u8 *pat, u8 n, u8 match_case);

/* Chargement par morceaux : lire dans *tb_load_buf(&room) (au plus room
 * octets ; room = 0 : tampon plein), puis tb_load_commit(n) pour n octets
 * utiles. A appeler apres tb_clear(). La sauvegarde parcourt le texte avec
 * tb_ptr(). */
u8  *tb_load_buf(u16 *room);
void tb_load_commit(u16 n);

#endif
