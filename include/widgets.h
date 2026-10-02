/* widgets.h -- label, bouton, champ de saisie, liste. Tous statiques :
 * l'application declare la struct, la fonction *_init() la remplit. */
#ifndef TUI_WIDGETS_H
#define TUI_WIDGETS_H

#include "tview.h"

/* --- TLabel : texte statique ------------------------------------------- */
typedef struct {
    TView v;
    const char *text;
} TLabel;

/* w = 0 : largeur du texte. Une largeur fixe efface l'ancien texte lors
 * d'un label_set() plus court. */
void label_init(TLabel *l, u8 x, u8 y, u8 w, const char *text);
void label_set(TLabel *l, const char *text);

/* --- TButton : "< texte >", Entree/Espace emet la commande ------------- */
typedef struct {
    TView v;
    const char *text;
    u8 cmd;
} TButton;

void button_init(TButton *b, u8 x, u8 y, u8 w, const char *text, u8 cmd);

/* --- TInputLine : saisie mono-ligne ------------------------------------ */
typedef struct {
    TView v;
    char *buf;               /* max + 1 octets */
    u8 max, len, pos, scroll;
} TInputLine;

void input_init(TInputLine *in, u8 x, u8 y, u8 w, char *buf, u8 max);
void input_set_text(TInputLine *in, const char *s);

/* --- TListBox : liste scrollable --------------------------------------- */
typedef const char *(*TListGet)(void *ctx, u16 index);

typedef struct {
    TView v;
    TListGet get;
    void *ctx;
    u16 count, sel, top;
    u8 cmd;                  /* emise sur Entree (CM_NONE : rien) */
} TListBox;

void list_init(TListBox *l, u8 x, u8 y, u8 w, u8 h,
               TListGet get, void *ctx, u16 count, u8 cmd);
/* Accesseur pret a l'emploi pour un tableau de chaines (ctx = tableau). */
const char *list_strings_get(void *ctx, u16 index);
void list_set_count(TListBox *l, u16 count);   /* recadre selection, redessine */
void list_select(TListBox *l, u16 index);

/* --- TCheckBox : "(x) texte", Espace/Entree/clic bascule ---------------- */
typedef struct {
    TView v;
    const char *text;
    u8 checked;
} TCheckBox;

void checkbox_init(TCheckBox *c, u8 x, u8 y, const char *text, u8 checked);

/* --- TRadioGroup : boutons radio, choix exclusif ------------------------ */
typedef struct {
    TView v;
    const char * const *labels;   /* count entrees, duree de vie >= la vue */
    u8 count, sel;
} TRadioGroup;

/* h = count (une ligne par option, pas de defilement). sel >= count : 0. */
void radiogroup_init(TRadioGroup *r, u8 x, u8 y, u8 w,
                     const char * const *labels, u8 count, u8 sel);
void radiogroup_select(TRadioGroup *r, u8 sel);

/* --- TProgress : barre de progression ----------------------------------- */
typedef struct {
    TView v;
    u16 value, max;
} TProgress;

/* max = 0 : barre vide en permanence (evite une division par zero tant que
 * la taille totale n'est pas encore connue). */
void progress_init(TProgress *p, u8 x, u8 y, u8 w, u16 max);
/* Borne a max. Cellules remplies = espace en video inverse ; vides = '.'. */
void progress_set(TProgress *p, u16 value);

#endif
