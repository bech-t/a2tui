/* dialog.c -- constructeur de dialogues a partir d'une table. */

#include <string.h>
#include "dialog.h"
#include "screen.h"

/* Le code des dialogues va dans la carte langage (segment LC de cc65) : de la
 * place gagnee dans la RAM principale. Il n'appelle ni la ROM ni le moniteur. */
#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

TWindow    dlg_win;
TDlgWidget dlg_item[DLG_MAX];

void dialog_build(const char *title, u8 w, u8 h, const TDlgItem *items, u8 n,
                  u8 default_cmd)
{
    u8 i;
    TView *v;
    TDlgWidget *d = dlg_item;

    window_init(&dlg_win, 0, 0, w, h, title, WF_MODAL);
    window_center(&dlg_win);
    for (i = 0; i < n; ++i, ++items, ++d) {
        switch (items->kind) {
        case DI_LABEL:
            label_init(&d->label, items->x, items->y, items->w, items->text);
            v = &d->label.v;
            break;
        case DI_BUTTON:
            button_init(&d->button, items->x, items->y, items->w, items->text, items->arg);
            v = &d->button.v;
            break;
        case DI_INPUT:
            input_init(&d->input, items->x, items->y, items->w, (char *)items->text, items->arg);
            v = &d->input.v;
            break;
        case DI_LIST:
            list_init(&d->list, items->x, items->y, items->w, items->arg, 0, 0, 0, CM_NONE);
            v = &d->list.v;
            break;
        default:
            checkbox_init(&d->check, items->x, items->y, items->text, items->arg);
            v = &d->check.v;
            break;
        }
        window_add(&dlg_win, v);
    }
    window_set_default(&dlg_win, default_cmd);
}

/* --- msgbox ------------------------------------------------------------- */

#define MB_LINES 4

/* Boutons : [OK 8] [OK 10, Cancel 10] [Yes, No, Cancel] -- les genres
 * MB_OK..MB_YESNOCANCEL y puisent des tranches contigues. */
static const struct { const char *text; u8 cmd, w; } mb_btn[] = {
    { "OK", CM_OK, 8 },
    { "OK", CM_OK, 10 }, { "Cancel", CM_CANCEL, 10 },
    { "Yes", CM_YES, 7 }, { "No", CM_NO, 7 }, { "Cancel", CM_CANCEL, 10 },
};
static const u8 mb_first[] = { 0, 1, 3, 3 };
static const u8 mb_count[] = { 1, 2, 2, 3 };

static TDlgItem mb_items[MB_LINES + 3];
static char     mb_text[128];

u8 msgbox(const char *title, const char *text, u8 kind)
{
    u8 n = 0, i, len, maxw = 0, bw = 0, width, x, nb = mb_count[kind];
    const u8 *first = mb_first + kind;
    char *p = mb_text;
    TDlgItem *it = mb_items;

    strncpy(mb_text, text, sizeof mb_text - 1);
    for (;;) {                                  /* une ligne = un libelle */
        if (n < MB_LINES) {
            it->kind = DI_LABEL;
            it->x = 2;
            it->y = (u8)(1 + n);
            it->w = 0;
            it->text = p;
            it->arg = 0;
            ++it;
            ++n;
        }
        while (*p && *p != '\n')
            ++p;
        len = (u8)(p - (it - 1)->text);
        if (len > maxw)
            maxw = len;
        if (!*p)
            break;
        *p++ = '\0';
    }
    for (i = 0; i < nb; ++i)
        bw += (u8)(mb_btn[*first + i].w + 1);
    --bw;
    width = (u8)((maxw > bw ? maxw : bw) + 4);
    if (width > scr_cols - 2)
        width = (u8)(scr_cols - 2);
    x = (u8)((width - bw) / 2);
    for (i = 0; i < nb; ++i, ++it) {
        it->kind = DI_BUTTON;
        it->x = x;
        it->y = (u8)(n + 2);
        it->w = mb_btn[*first + i].w;
        it->text = mb_btn[*first + i].text;
        it->arg = mb_btn[*first + i].cmd;
        x = (u8)(x + it->w + 1);
    }
    dialog_build(title, width, (u8)(n + 4), mb_items, (u8)(n + nb), mb_btn[*first].cmd);
    return dialog_run();
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
