/* editor.h -- widget d'edition de texte au-dessus du gap buffer.
 * Une seule instance (`ed`) : l'etat est global, ce qui donne un code bien
 * plus compact sous cc65 qu'un acces par pointeur.
 *
 * Clavier Apple II, sans Shift/Home/Fin :
 *   fleches                deplacement         ^A / ^E   debut / fin de ligne
 *   Pomme fermee + mvt     etend la selection  ^T / ^B   debut / fin du texte
 *   Pomme ouverte + gauche/droite  mot ; + haut/bas  page
 *   Suppr ($7F)            efface a gauche     ^D        efface a droite
 * Souris : clic place le curseur, glisser selectionne. */
#ifndef EDITOR_H
#define EDITOR_H

#include "tview.h"

#define CLIP_MAX 512

typedef struct TEditor {
    TView v;
    u16 caret, anchor;            /* offsets ; selection = [anchor, caret) ou inverse */
    u8  sel;                      /* selection active */
    u16 top, top_line;            /* debut de la 1re ligne visible ; son numero (1..) */
    u16 line, anchor_line;        /* numero de ligne du curseur / de l'ancre */
    u16 nlines;                   /* nombre total de lignes */
    u16 col, want_col, hscroll;   /* colonne du curseur ; colonne visee ; defilement horizontal */
    u8  modified;
    u8  dragging;
    u8  dlo, dhi, dall;           /* lignes ecran a redessiner */
    void (*notify)(void);         /* position / etat modifies */
} TEditor;

extern TEditor ed;

void ed_init(u8 x, u8 y, u8 w, u8 h);
/* Apres tb_set_loaded()/tb_clear() : repart en haut du texte. */
void ed_reset(void);

u8   ed_insert(const u8 *s, u16 n);   /* 0 si tampon plein */
void ed_backspace(void);
void ed_delete(void);
void ed_delete_line(void);

/* Presse-papiers : 0 = ok, 1 = rien de selectionne, 2 = trop gros, 3 = vide/plein */
u8   ed_copy(void);
u8   ed_cut(void);
u8   ed_paste(void);
void ed_clear_sel(void);
void ed_select_all(void);
u8   ed_has_sel(void);
u16  ed_sel_len(void);

/* Recherche depuis le curseur (avec retour au debut) ; selectionne l'occurrence. */
u8   ed_find(const u8 *pat, u8 n, u8 match_case);
/* Remplace la selection si elle vaut pat ; renvoie 1 si remplacee. */
u8   ed_replace(const u8 *pat, u8 n, u8 match_case, const u8 *rep, u8 rn);
u16  ed_replace_all(const u8 *pat, u8 n, u8 match_case, const u8 *rep, u8 rn);
void ed_goto_line(u16 line);

#endif
