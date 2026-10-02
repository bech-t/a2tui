/* screen.h -- couche Rendu : tampon virtuel + shadow, flush differentiel.
 * Seule couche connaissant la memoire video de l'Apple II. */
#ifndef TUI_SCREEN_H
#define TUI_SCREEN_H

#include "tui_config.h"

#define ATTR_NORMAL  0
#define ATTR_INVERSE 1
#define ATTR_FLASH   2   /* rendu en inverse : le jeu alternatif n'a pas de flash */

/* Glyphes logiques (a passer comme `ch`). Rendus en MouseText si disponible
 * (et TUI_MOUSETEXT), sinon en ASCII ; toujours en ASCII en video inverse.
 * Le cadre MouseText n'a pas de glyphes de coin : les bordures haute/basse
 * sont des lignes en bas/haut de cellule qui rejoignent les barres laterales. */
#define G_TL     0x80   /* coin haut-gauche  */
#define G_TR     0x81
#define G_BL     0x82
#define G_BR     0x83
#define G_T      0x84   /* bordure haute */
#define G_B      0x85
#define G_L      0x86   /* bordure gauche */
#define G_R      0x87
#define G_HSEP   0x88   /* separateur horizontal (menu) */
#define G_UP     0x89
#define G_DOWN   0x8A
#define G_LEFT   0x8B
#define G_RIGHT  0x8C
#define G_CHECK  0x8D
#define G_TRACK  0x8E   /* piste de barre de defilement */
#define G_COUNT  15

extern u8 scr_cols;              /* 40 ou 80, fixe apres scr_init() */

/* Detecte la carte 80 colonnes, configure le mode texte, efface l'ecran. */
void scr_init(void);
/* Rend un ecran texte 40 colonnes propre (avant de quitter). */
void scr_done(void);

/* Adresse physique de la colonne 0 de la ligne y (page texte 1). */
u16  scr_addr(u8 y);

/* Primitives de dessin : n'ecrivent QUE dans le tampon virtuel. Tout ce qui
 * sort de l'ecran est ignore. */
void scr_putc(u8 x, u8 y, char ch, u8 attr);
void scr_puts(u8 x, u8 y, const char *s, u8 attr);
/* Ecrit s tronquee/completee d'espaces sur exactement w cellules. */
void scr_putsw(u8 x, u8 y, const char *s, u8 w, u8 attr);
void scr_fill(u8 x, u8 y, u8 w, u8 h, char ch, u8 attr);
/* Rectangle efface (espaces) avec cadre : glyphes G_TL... */
void scr_box(u8 x, u8 y, u8 w, u8 h);

/* Fait defiler le rectangle de |n| lignes dans le tampon virtuel (n > 0 : le
 * contenu monte). Les lignes decouvertes gardent leur ancien contenu : a
 * l'appelant de les redessiner. Toutes les lignes du rectangle sont a reecrire. */
void scr_scroll(u8 x, u8 y, u8 w, u8 h, signed char n);

/* Sauvegarde/restauration d'un rectangle (codes ecran bruts, w*h octets). */
void scr_save(u8 x, u8 y, u8 w, u8 h, u8 *buf);
void scr_restore(u8 x, u8 y, u8 w, u8 h, const u8 *buf);

/* Pointeur de souris : superpose a l'ecran a la fin de chaque flush, sans
 * toucher au tampon virtuel (MouseText : fleche ; sinon cellule inversee). */
void scr_mouse_move(u8 x, u8 y);      /* deplace et affiche */
void scr_mouse_show(u8 on);

/* Pousse vers la memoire video les seules cellules modifiees. */
void scr_flush(void);
/* Force la reecriture complete au prochain flush (ecran corrompu de l'exterieur). */
void scr_invalidate(void);

#ifdef TUI_HOST
/* Memoire video simulee ($0400-$07FF, principale et auxiliaire) et reglages
 * a poser AVANT scr_init() (version hote, PC). */
extern u8 tui_sim_main[1024], tui_sim_aux[1024];
extern u8 tui_sim_has_aux, tui_sim_has_mousetext;
extern unsigned long tui_sim_writes;
#endif

#endif
