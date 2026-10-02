/* tui_config.h -- dimensionnement statique de la bibliotheque.
 * Surchargeable a la compilation : -DTUI_SCR_MAXCOLS=40 divise par deux les
 * tampons ecran (ecran 40 colonnes seulement). */
#ifndef TUI_CONFIG_H
#define TUI_CONFIG_H

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;

/* Largeur maximale supportee (40 ou 80). Fixe la taille des tampons. */
#ifndef TUI_SCR_MAXCOLS
#define TUI_SCR_MAXCOLS 80
#endif
#define TUI_SCR_ROWS    24
/* Ligne 0 : barre de menu. Ligne 23 : barre d'etat. Entre : le bureau. */
#define TUI_DESK_ROWS   (TUI_SCR_ROWS - 2)

/* Enfants maximum par groupe (fenetre, bureau). */
#ifndef TUI_MAX_CHILDREN
#define TUI_MAX_CHILDREN 12
#endif

/* Taille (en cellules) du tampon de sauvegarde d'un menu deroulant. */
#ifndef TUI_MENU_SAVE
#define TUI_MENU_SAVE 300
#endif

/* Glyphes semi-graphiques MouseText (cadres, fleches, coche...). 1 : utilises
 * si la machine les possede (//e enhanced, //e carte, //c ; detecte a
 * l'execution), sinon repli ASCII automatique. 0 : ASCII uniquement, et le
 * code MouseText n'est pas compile. */
#ifndef TUI_MOUSETEXT
#define TUI_MOUSETEXT 1
#endif

/* Souris : 1 = driver cc65 (carte souris Apple, port //c), pointeur MouseText,
 * clics sur les widgets et les menus. 0 = clavier seul (le driver n'est pas lie). */
#ifndef TUI_MOUSE
#define TUI_MOUSE 1
#endif

/* 1 : la fleche gauche ($08) efface aussi en fin de saisie -- II/II+ sans
 * touche Suppr. Par defaut, $08 deplace le curseur et $7F efface. */
#ifndef TUI_LEFT_IS_BS
#define TUI_LEFT_IS_BS 0
#endif

#endif
