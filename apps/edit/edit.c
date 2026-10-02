/* edit.c -- editeur de texte pour Apple II, dans l'esprit de MS-DOS EDIT. */

#include <string.h>
#include "a2tui.h"
#include "editor.h"
#include "textbuf.h"
#include "edfile.h"

#define CM_NEW      (CM_USER + 0)
#define CM_OPEN     (CM_USER + 1)
#define CM_SAVE     (CM_USER + 2)
#define CM_SAVEAS   (CM_USER + 3)
#define CM_CUT      (CM_USER + 4)
#define CM_COPY     (CM_USER + 5)
#define CM_PASTE    (CM_USER + 6)
#define CM_CLEAR    (CM_USER + 7)
#define CM_SELALL   (CM_USER + 8)
#define CM_DELLINE  (CM_USER + 9)
#define CM_FIND     (CM_USER + 10)
#define CM_FINDNEXT (CM_USER + 11)
#define CM_CHANGE   (CM_USER + 12)
#define CM_GOTO     (CM_USER + 13)
#define CM_KEYS     (CM_USER + 14)
#define CM_ABOUT    (CM_USER + 15)

/* --- Menus ------------------------------------------------------------- */

static const TMenuItem file_items[] = {
    { "New",        CM_NEW,    KEY_CTRL('N') },
    { "Open...",    CM_OPEN,   KEY_CTRL('O') },
    { "Save",       CM_SAVE,   KEY_CTRL('S') },
    { "Save As...", CM_SAVEAS, 0 },
    { "-",          CM_NONE,   0 },
    { "Exit",       CM_QUIT,   KEY_CTRL('Q') },
};
static const TMenuItem edit_items[] = {
    { "Cut",         CM_CUT,     KEY_CTRL('X') },
    { "Copy",        CM_COPY,    KEY_CTRL('C') },
    { "Paste",       CM_PASTE,   KEY_CTRL('V') },
    { "Clear",       CM_CLEAR,   0 },
    { "-",           CM_NONE,    0 },
    { "Select all",  CM_SELALL,  KEY_CTRL('L') },
    { "Delete line", CM_DELLINE, KEY_CTRL('Y') },
};
static const TMenuItem search_items[] = {
    { "Find...",       CM_FIND,     KEY_CTRL('F') },
    { "Repeat find",   CM_FINDNEXT, KEY_CTRL('G') },
    { "Change...",     CM_CHANGE,   KEY_CTRL('R') },
    { "Go to line...", CM_GOTO,     KEY_CTRL('W') },
};
static const TMenuItem help_items[] = {
    { "Keys...",  CM_KEYS,  0 },
    { "About...", CM_ABOUT, 0 },
};
static const TMenu menus[] = {
    { "File",   file_items,   6 },
    { "Edit",   edit_items,   7 },
    { "Search", search_items, 4 },
    { "Help",   help_items,   2 },
};

static const char *const help_lines[] = {
    "Arrows           move",
    "Closed-Apple+key select text",
    "Open-Apple+< >   word left/right",
    "Open-Apple+^ v   page up/down",
    "^A  ^E           line start/end",
    "^T  ^B           text start/end",
    "Delete / ^D      erase left/right",
    "^Y               delete line",
    "^X  ^C  ^V       cut copy paste",
    "^L               select all",
    "^F  ^G  ^R       find again change",
    "^W               go to line",
    "^N  ^O  ^S       new open save",
    "^Q               exit",
    "Esc              menu",
    "Mouse            click, drag = select",
};

/* --- Etat -------------------------------------------------------------- */

static TWindow win;
static TMenuBar mbar;
static char cur_name[FS_PATH_LEN];
static u8   has_name;
static char title_buf[40];
static char status_buf[81];

static char find_buf[41], rep_buf[41];
static u8   find_case;

/* --- Utilitaires ------------------------------------------------------- */

static char *put_str(char *p, const char *s)
{
    while (*s)
        *p++ = *s++;
    return p;
}

/* Decimal sans division (la division 16 bits de cc65 coute ~400 cycles). */
static char *put_u16(char *p, u16 v)
{
    static const u16 pow10[4] = { 10000, 1000, 100, 10 };
    u8 i, d, started = 0;
    for (i = 0; i < 4; ++i) {
        for (d = 0; v >= pow10[i]; ++d)
            v = (u16)(v - pow10[i]);
        if (d || started)
            *p++ = (char)('0' + d), started = 1;
    }
    *p++ = (char)('0' + v);
    return p;
}

/* Met en majuscules le nom du fichier (derniere composante du chemin) : les
 * dossiers viennent du disque, tels quels. */
static void upcase(char *s)
{
    char *slash = strrchr(s, '/');
    if (slash)
        s = slash + 1;
    for (; *s; ++s)
        if (*s >= 'a' && *s <= 'z')
            *s -= 32;
}

static void update_status(void)
{
    char *p = status_buf;
    p = put_str(p, " Ln ");
    p = put_u16(p, ed.line);
    p = put_str(p, "/");
    p = put_u16(p, ed.nlines);
    p = put_str(p, "  Col ");
    p = put_u16(p, (u16)(ed.col + 1));
    p = put_str(p, "  Free ");
    p = put_u16(p, tb_free());
    if (ed.modified)
        p = put_str(p, "  Modified");
    *p = '\0';
    app_set_status(status_buf);
}

static void set_title(void)
{
    char *p = put_str(title_buf, "EDIT - ");
    p = put_str(p, !has_name ? "Untitled"
                : strrchr(cur_name, '/') ? strrchr(cur_name, '/') + 1 : cur_name);
    *p = '\0';
    win.title = title_buf;
    view_invalidate(&win.g.v);
}

static void note(const char *title, const char *text);

static void note(const char *title, const char *text)
{
    msgbox(title, text, MB_OK);
}

/* --- Dialogues (tables : voir dialog.h) ------------------------------- */

static const TFileSelText fs_text = {
    "Name:", "OK", "Cancel", "New", "File exists.\nOverwrite it?", "Cannot create\nthe folder",
};

/* name : entree/sortie (FS_PATH_LEN octets). Renvoie CM_OK ou CM_CANCEL. */
static u8 file_dialog(const char *title, char *name, u8 saving)
{
    return filesel(saving ? FS_SAVE : FS_OPEN, title, &fs_text, name);
}

/* --- Fichiers ---------------------------------------------------------- */

static char err_buf[40];

/* Message d'erreur ; le code systeme (ProDOS, en hexadecimal) est ajoute. */
static const char *err_text(u8 e)
{
    char *p = put_str(err_buf, e == ED_ERR_BIG ? "File too large\nfor the buffer"
                             : e == ED_ERR_OPEN ? "Cannot open file" : "Disk error");
    if (ed_os_error && e != ED_ERR_BIG) {
        p = put_str(p, "\nProDOS error $");
        *p++ = "0123456789ABCDEF"[ed_os_error >> 4];
        *p++ = "0123456789ABCDEF"[ed_os_error & 15];
    }
    *p = '\0';
    return err_buf;
}

static u8 write_out(const char *name)
{
    u8 r = ed_save_file(name);
    if (r != ED_OK) {
        note("Error", err_text(r));
        return 0;
    }
    ed.modified = 0;
    update_status();
    return 1;
}

static u8 do_saveas(void)
{
    char name[FS_PATH_LEN];
    strcpy(name, has_name ? cur_name : "");
    if (file_dialog("Save As", name, 1) != CM_OK)
        return 0;
    upcase(name);
    if (name[0] == '\0')
        return 0;
    if (!write_out(name))
        return 0;
    strcpy(cur_name, name);
    has_name = 1;
    set_title();
    return 1;
}

static u8 do_save(void)
{
    return has_name ? write_out(cur_name) : do_saveas();
}

/* 1 si on peut continuer (rien a perdre, enregistre, ou abandon voulu). */
static u8 confirm_discard(void)
{
    u8 r;
    if (!ed.modified)
        return 1;
    r = msgbox("Edit", "Save changes?", MB_YESNOCANCEL);
    if (r == CM_YES)
        return do_save();
    return r == CM_NO;
}

static void do_new(void)
{
    if (!confirm_discard())
        return;
    tb_clear();
    ed_reset();
    has_name = 0;
    set_title();
}

static void do_open(void)
{
    char name[FS_PATH_LEN];
    u8 r;
    if (!confirm_discard())
        return;
    name[0] = '\0';
    if (file_dialog("Open", name, 0) != CM_OK || name[0] == '\0')
        return;
    upcase(name);
    r = ed_load_file(name);
    if (r != ED_OK) {
        note("Error", err_text(r));
        if (r != ED_ERR_OPEN) {            /* le tampon a ete vide */
            ed_reset();
            has_name = 0;
            set_title();
        }
        return;
    }
    ed_reset();
    strcpy(cur_name, name);
    has_name = 1;
    set_title();
}

/* --- Recherche --------------------------------------------------------- */

static const TDlgItem find_items[] = {
    { DI_LABEL,  2, 1,  0, "Find what:", 0 },
    { DI_INPUT,  2, 2, 33, find_buf,    40 },
    { DI_CHECK,  2, 4,  0, "Match case", 0 },
#define F_CASE 2
    { DI_BUTTON, 6, 6, 10, "OK",         CM_OK },
    { DI_BUTTON, 22, 6, 12, "Cancel",    CM_CANCEL },
};

static const TDlgItem change_items[] = {
    { DI_LABEL,  2, 1,  0, "Find what:", 0 },
    { DI_INPUT,  2, 2, 35, find_buf,    40 },
    { DI_LABEL,  2, 4,  0, "Change to:", 0 },
    { DI_INPUT,  2, 5, 35, rep_buf,     40 },
    { DI_CHECK,  2, 7,  0, "Match case", 0 },
#define C_CASE 4
    { DI_BUTTON, 2, 9,  8, "Find",       CM_OK },
    { DI_BUTTON, 11, 9, 11, "Replace",   CM_YES },
    { DI_BUTTON, 23, 9,  7, "All",       CM_NO },
    { DI_BUTTON, 31, 9,  7, "Esc",       CM_CANCEL },
};

static void find_next(void)
{
    u8 n = (u8)strlen(find_buf);
    if (n == 0) {
        note("Find", "Nothing to find");
        return;
    }
    if (!ed_find((const u8 *)find_buf, n, find_case))
        note("Find", "Not found");
}

static void do_find(void)
{
    dialog_build("Find", 38, 8, find_items, 5, CM_OK);
    input_set_text(&dlg_item[1].input, find_buf);
    dlg_item[F_CASE].check.checked = find_case;
    if (dialog_run() == CM_OK) {
        find_case = dlg_item[F_CASE].check.checked;
        find_next();
    }
}

static void do_change(void)
{
    u16 n;
    char msg[24], *p;
    for (;;) {
        dialog_build("Change", 40, 11, change_items, 9, CM_OK);
        input_set_text(&dlg_item[1].input, find_buf);
        input_set_text(&dlg_item[3].input, rep_buf);
        dlg_item[C_CASE].check.checked = find_case;
        n = dialog_run();
        find_case = dlg_item[C_CASE].check.checked;
        if (n == CM_CANCEL)
            return;
        if (find_buf[0] == '\0')
            continue;
        if (n == CM_OK) {
            find_next();
        } else if (n == CM_YES) {
            ed_replace((const u8 *)find_buf, (u8)strlen(find_buf), find_case,
                       (const u8 *)rep_buf, (u8)strlen(rep_buf));
            find_next();
        } else {
            n = ed_replace_all((const u8 *)find_buf, (u8)strlen(find_buf),
                               find_case, (const u8 *)rep_buf, (u8)strlen(rep_buf));
            p = put_u16(msg, n);
            p = put_str(p, " replaced");
            *p = '\0';
            note("Change", msg);
        }
    }
}

static char gw_buf[8];

static const TDlgItem goto_items[] = {
    { DI_LABEL,  2, 1,  0, "Line number:", 0 },
    { DI_INPUT,  2, 2,  8, gw_buf,         6 },
    { DI_BUTTON, 3, 4, 10, "OK",           CM_OK },
    { DI_BUTTON, 15, 4, 12, "Cancel",      CM_CANCEL },
};

static void do_goto(void)
{
    u16 n = 0;
    char *p;
    dialog_build("Go to line", 30, 7, goto_items, 4, CM_OK);
    if (dialog_run() != CM_OK)
        return;
    for (p = gw_buf; *p >= '0' && *p <= '9'; ++p)
        n = (u16)(n * 10 + (*p - '0'));
    if (n)
        ed_goto_line(n);
}

/* --- Aide -------------------------------------------------------------- */

static const TDlgItem keys_items[] = {
    { DI_LIST,   2,  1, 36, 0, 14 },
    { DI_BUTTON, 15, 16, 10, "OK", CM_OK },
};

static void do_keys(void)
{
    dialog_build("Keys", 40, 18, keys_items, 2, CM_OK);
    dlg_item[0].list.get = list_strings_get;
    dlg_item[0].list.ctx = (void *)help_lines;
    list_set_count(&dlg_item[0].list, sizeof help_lines / sizeof help_lines[0]);
    dialog_run();
}

/* --- Commandes --------------------------------------------------------- */

static void clip_error(u8 r)
{
    if (r == 1)
        note("Edit", "Nothing selected");
    else if (r == 2)
        note("Edit", "Selection too large\nfor the clipboard");
    else if (r == 3)
        note("Edit", "Nothing to paste\nor buffer full");
}

static u8 on_command(TEvent *ev)
{
    switch (ev->cmd) {
    case CM_NEW:      do_new();                     return EVENT_HANDLED;
    case CM_OPEN:     do_open();                    return EVENT_HANDLED;
    case CM_SAVE:     do_save();                    return EVENT_HANDLED;
    case CM_SAVEAS:   do_saveas();                  return EVENT_HANDLED;
    case CM_CUT:      clip_error(ed_cut());      return EVENT_HANDLED;
    case CM_COPY:     clip_error(ed_copy());     return EVENT_HANDLED;
    case CM_PASTE:    clip_error(ed_paste());    return EVENT_HANDLED;
    case CM_CLEAR:    ed_clear_sel();            return EVENT_HANDLED;
    case CM_SELALL:   ed_select_all();           return EVENT_HANDLED;
    case CM_DELLINE:  ed_delete_line();          return EVENT_HANDLED;
    case CM_FIND:     do_find();                    return EVENT_HANDLED;
    case CM_FINDNEXT: find_next();                  return EVENT_HANDLED;
    case CM_CHANGE:   do_change();                  return EVENT_HANDLED;
    case CM_GOTO:     do_goto();                    return EVENT_HANDLED;
    case CM_KEYS:     do_keys();                    return EVENT_HANDLED;
    case CM_ABOUT:
        note("About", "EDIT for Apple II\nbuilt on a2tui");
        return EVENT_HANDLED;
    case CM_QUIT:
        return confirm_discard() ? EVENT_NOT_HANDLED : EVENT_HANDLED;
    }
    return EVENT_NOT_HANDLED;
}

/* Construit l'interface ; app_run() est lance par main(). */
void edit_setup(void)
{
    find_buf[0] = rep_buf[0] = '\0';
    find_case = 0;
    has_name = 0;
    tb_init();
    app_init();
    menubar_init(&mbar, menus, 4);
    app_set_menubar(&mbar);

    window_init(&win, 0, 0, scr_cols, TUI_DESK_ROWS, "EDIT - Untitled", 0);
    ed_init(1, 1, (u8)(scr_cols - 2), (u8)(TUI_DESK_ROWS - 2));
    ed.notify = update_status;
    window_add(&win, &ed.v);
    app_insert(&win.g.v);
    ed_reset();
    set_title();

    app_set_handler(on_command);
}

#ifndef EDIT_TEST
int main(void)
{
    edit_setup();
    app_run();
    app_done();
    return 0;
}
#endif
