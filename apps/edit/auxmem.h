/* auxmem.h -- RAM auxiliaire de l'Apple //e / //c (64 Ko) utilisee comme
 * stockage de masse pour le tampon de texte.
 *
 * Zone utilisee : $0800-$BFFF de la banque auxiliaire (AUX_CAP octets) ; la
 * page texte ($0400-$07FF) reste a l'affichage 80 colonnes. Les adresses des
 * fonctions ci-dessous sont des decalages depuis le debut de cette zone. */
#ifndef AUXMEM_H
#define AUXMEM_H

#include "tui_config.h"

#define AUX_CAP 47104u

/* Verifie la presence de la RAM auxiliaire et s'en empare : renvoie 1 si elle
 * est utilisable. Sous ProDOS, le disque /RAM l'occupe : il n'est repris que
 * s'il est vide, et il est alors debranche de la liste des peripheriques
 * (son contenu ne survit pas). */
u8   aux_init(void);

void aux_read(u8 *dst, u16 off, u16 n);               /* aux -> principale */
void aux_write(u16 off, const u8 *src, u16 n);        /* principale -> aux */
void aux_move(u16 dst, u16 src, u16 n);               /* aux -> aux, chevauchement admis */

#ifdef TUI_HOST
/* Version hote (PC) : la RAM auxiliaire est un tableau, activee par cet
 * indicateur avant tb_init(). */
extern u8 aux_host_enable;
#endif

#endif
