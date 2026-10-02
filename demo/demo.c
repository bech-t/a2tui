/* demo.c -- vitrine de la bibliotheque : menus, fenetre, saisie, liste,
 * case a cocher, boutons radio, barre de progression, dialogue modal, msgbox.
 * Tient en 40 colonnes comme en 80. */

#include <stdio.h>
#include <string.h>
#include "a2tui.h"

#define CM_ABOUT   (CM_USER + 0)
#define CM_DIALOG  (CM_USER + 1)
#define CM_PICK    (CM_USER + 2)
#define CM_CLEAR   (CM_USER + 3)
#define CM_STEP    (CM_USER + 4)

static const TMenuItem file_items[] = {
    { "About...",  CM_ABOUT, 0 },
    { "-",         CM_NONE,  0 },
    { "Quit",      CM_QUIT,  KEY_CTRL('Q') },
};
static const TMenuItem tools_items[] = {
    { "Dialog...", CM_DIALOG, KEY_CTRL('O') },
    { "Clear name", CM_CLEAR, 0 },
    { "Step progress", CM_STEP, 0 },
};
static const TMenu menus[] = {
    { "File",  file_items,  3 },
    { "Tools", tools_items, 3 },
};
static TMenuBar menubar;

static const char *const utilities[] = {
    "Text editor", "Disk formatter", "Disk copy", "Serial transfer",
    "Catalog", "Launcher", "Settings",
};
static const char *const priorities[] = { "Low", "Normal", "High" };

static TWindow win;
static TLabel lbl_name, lbl_list, lbl_prog, lbl_res, lbl_mode;
static TInputLine in_name;
static char name_buf[21];
static TListBox list;
static TCheckBox chk_enable;
static TRadioGroup radio_prio;
static TProgress prog;
static TButton btn_dlg, btn_quit;
static char res_buf[36];

static TWindow dlg;
static TLabel dlg_lbl;
static TInputLine dlg_in;
static char dlg_buf[17];
static TButton dlg_ok, dlg_cancel;

static void show_result(const char *what, const char *arg)
{
    sprintf(res_buf, "%s: %s", what, arg);
    label_set(&lbl_res, res_buf);
}

static void open_dialog(void)
{
    input_set_text(&dlg_in, name_buf);
    if (app_exec(&dlg) == CM_OK) {
        input_set_text(&in_name, dlg_buf);
        show_result("Dialog OK", dlg_buf);
    } else {
        show_result("Dialog", "cancelled");
    }
}

static u8 on_command(TEvent *ev)
{
    switch (ev->cmd) {
    case CM_ABOUT:
        msgbox("About", "a2tui demo\nTurbo Vision-like TUI\nfor Apple II", MB_OK);
        return EVENT_HANDLED;
    case CM_DIALOG:
        open_dialog();
        return EVENT_HANDLED;
    case CM_CLEAR:
        input_set_text(&in_name, "");
        return EVENT_HANDLED;
    case CM_STEP: {
        u16 v = (u16)(prog.value + 2);
        progress_set(&prog, v > prog.max ? 0 : v);
        return EVENT_HANDLED;
    }
    case CM_PICK:
        show_result("Picked", utilities[list.sel]);
        return EVENT_HANDLED;
    case CM_QUIT:
        if (msgbox("Quit", "Really quit?", MB_YESNO) != CM_YES)
            return EVENT_HANDLED;
        break;
    }
    return EVENT_NOT_HANDLED;
}

int main(void)
{
    static char mode_buf[24];

    app_init();

    menubar_init(&menubar, menus, 2);
    app_set_menubar(&menubar);
    app_set_status(" Esc:menu  Tab:next  Enter:select  ^Q:quit");

    window_init(&win, 1, 1, 36, 20, "Utilities", 0);
    label_init(&lbl_name, 2, 2, 0, "Name:");
    input_init(&in_name, 8, 2, 20, name_buf, 20);
    label_init(&lbl_list, 2, 4, 0, "Choose a tool:");
    list_init(&list, 2, 5, 32, 5, list_strings_get, (void *)utilities, 7, CM_PICK);
    checkbox_init(&chk_enable, 2, 10, "Enabled", 0);
    radiogroup_init(&radio_prio, 2, 11, 20, priorities, 3, 1);
    label_init(&lbl_prog, 2, 14, 0, "Progress:");
    progress_init(&prog, 2, 15, 20, 10);
    label_init(&lbl_res, 2, 16, 32, "");
    button_init(&btn_dlg, 2, 18, 12, "Dialog", CM_DIALOG);
    button_init(&btn_quit, 16, 18, 10, "Quit", CM_QUIT);
    sprintf(mode_buf, mouse_present() ? "%uc+M" : "%uc", (unsigned)scr_cols);
    label_init(&lbl_mode, 28, 18, 0, mode_buf);
    window_add(&win, &lbl_name.v);
    window_add(&win, &in_name.v);
    window_add(&win, &lbl_list.v);
    window_add(&win, &list.v);
    window_add(&win, &chk_enable.v);
    window_add(&win, &radio_prio.v);
    window_add(&win, &lbl_prog.v);
    window_add(&win, &prog.v);
    window_add(&win, &lbl_res.v);
    window_add(&win, &btn_dlg.v);
    window_add(&win, &btn_quit.v);
    window_add(&win, &lbl_mode.v);
    app_insert(&win.g.v);

    window_init(&dlg, 0, 0, 30, 8, "Rename", WF_MODAL);
    window_center(&dlg);
    label_init(&dlg_lbl, 2, 2, 0, "New name:");
    input_init(&dlg_in, 2, 3, 16, dlg_buf, 16);
    button_init(&dlg_ok, 4, 5, 8, "OK", CM_OK);
    button_init(&dlg_cancel, 15, 5, 10, "Cancel", CM_CANCEL);
    window_add(&dlg, &dlg_lbl.v);
    window_add(&dlg, &dlg_in.v);
    window_add(&dlg, &dlg_ok.v);
    window_add(&dlg, &dlg_cancel.v);
    window_set_default(&dlg, CM_OK);

    app_set_handler(on_command);
    app_run();
    app_done();
    return 0;
}
