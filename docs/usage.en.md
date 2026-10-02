# a2tui: usage guide and technical notes

[Version française](usage.fr.md) · [README](../README.en.md)

A C (cc65) text-mode user interface library in the style of Turbo Vision, for
the Apple II. **Minimum: Apple //e or //c, 64 KB** (not the II or II+). 40 or
80 columns, detected at startup.

![a2tui demo in 80 columns (Apple //e)](demo.png)

## Building and a first program

```
make            # build/a2tui.lib
make dsk        # build/tuidemo.dsk: bootable ProDOS disk with the demo
make edit-dsk   # build/edit.dsk: the text editor (apps/edit), see below
```

Requirements: cc65, Java and AppleCommander (see `tools/ac/README.md`).
`prodos/` holds the template disk (ProDOS 2.4.2, open source) and the loader
taken from a2adv.

An application:

```c
#include "a2tui.h"

static TWindow win;  static TLabel lbl;  static TButton ok;

int main(void) {
    app_init();
    window_init(&win, 2, 2, 30, 8, "Hello", 0);
    label_init(&lbl, 2, 2, 0, "Hello Apple II");
    button_init(&ok, 2, 5, 10, "Quit", CM_QUIT);
    window_add(&win, &lbl.v);  window_add(&win, &ok.v);
    app_insert(&win.g.v);
    app_run();
    app_done();
    return 0;
}
```

Everything is static (no `malloc`): the caller declares the structure and
`*_init()` fills it in. See `demo/demo.c` for a complete example.

The widget delimiters (`< >`, `( )`, `.`) only use characters that are the
same on every national character set of the Apple II.

## Semigraphics (MouseText)

`#define TUI_MOUSETEXT 1` (default, in `tui_config.h` or
`-DTUI_MOUSETEXT=0`): frames, menu separators and glyphs (`G_UP`, `G_DOWN`,
`G_CHECK`, `G_TRACK`…, see `screen.h`) are drawn in MouseText on the enhanced
//e, the //e card and the //c (detected with `get_ostype()`). On an original
//e, or in inverse video, they fall back to ASCII on their own (`+ - |`). At
0, the MouseText code and tables are not compiled.

MouseText has no corner glyphs: the top border is `_` (a line at the bottom
of the cell), the bottom one is glyph `$4C` (a line at the top), and the bars
`$5F`/`$5A` join them. As a trade-off, the library always uses the
**alternate** character set, which has no *flash* video (the input cursor is
therefore a fixed inverse cell) but offers inverse lowercase.

## Mouse

`#define TUI_MOUSE 1` (default; `-DTUI_MOUSE=0` for a mouse-less binary, 2.8
KB smaller on the demo). Standard cc65 driver (`a2.stdmou`: Apple II mouse
card in a slot, built-in //c mouse port), statically linked; if no mouse is
detected, the library works from the keyboard unchanged. `mouse_present()`
tells which case applies.

- Pointer: MouseText arrow (an inverse cell without MouseText), overlaid on
  the screen at every flush without touching the virtual buffer.
- Clicks (left button): focus + action on a button, input line (places the
  cursor), list (selection; a second click validates), menu bar (title,
  item; hovering the drop-down, a click elsewhere closes it). In a modal
  dialog, anything outside the dialog is inert.
- Events: `EV_MOUSE_DOWN/UP/MOVE` with `mouse_x/mouse_y` in text cells.

## Architecture

| Layer | Files | Role |
| --- | --- | --- |
| Rendering | `screen.c` | virtual buffer, dirty lines, differential flush. A cell = 1 byte = Apple II screen code (attribute included). The only module that knows video memory. |
| Events | `keyboard.c`, `mouse.c`, `event.c` | `event_poll()` → a single `TEvent` (keyboard first, then mouse). |
| Views | `tview.c`, `widgets.c`, `window.c`, `menu.c` | `TView`/`TGroup`, drawing per dirty view, focus, bubbling. |
| Dialogs | `dialog.c`, `filesel.c`, `busy.c` | table-driven dialogs, `msgbox`, file/folder selector, wait indicator. |
| Application | `app.c` | desktop, menu bar (row 0), status bar (row 23), main loop, modal `app_exec()`. |

Event contract: `handle()` returns `EVENT_HANDLED` or `EVENT_NOT_HANDLED`. A
widget that emits a command **rewrites** the event as `EV_COMMAND` and returns
`NOT_HANDLED`: the command bubbles up from parent to parent to a modal
dialog, then to the application handler (`app_set_handler`).

Keyboard: Tab/Down/Right = next focus, Up/Left = previous (if the widget does
not take the key), Esc = opens the menu (or cancels a dialog), Enter = the
default button. Menu shortcuts: Ctrl-letter (except `^H ^I ^J ^K ^M ^U ^[`,
which are the arrows/Tab/Enter/Esc).

## Link granularity

Each widget (`label.c`, `button.c`, `input.c`, `list.c`, `checkbox.c`),
`dialog.c` and `menu.c` are separate files: `ld65` only links the modules that
are really referenced, so an application using no list, menu or dialog does
not pay for their code. Checked by linking a test binary that only used a
window/label/button: `menu.o`, `list.o`, `input.o`, `dialog.o` and
`checkbox.o` are absent from it.

A pitfall if you add a module: the linker prunes at the level of the **whole
object file**, not per function. If `app.c` called `menubar_run()` by name,
even in a branch never taken at run time, `menu.o` would be linked anyway:
the `if (menubar)` test is a RUN-TIME decision, not an absence of reference
at COMPILE time. That is why `app.c` only knows the `TMenuHooks` type
(menu.h) from `menu.c`: `menubar_init()` registers itself with `app.c` via
`app_set_menu_hooks()`, which moves all the coupling to the caller (which, if
it wants a menu, already links `menu.o` for that reason).

## Design choices

- `TView` holds a pointer to a **shared `const TViewOps` table** rather than
  two function pointers per instance (−2 bytes per view, same effect).
- A screen cell is **one byte** (screen code, attribute included): 80×24
  buffers = 1,920 bytes. Attributes: normal / inverse ("flash" is rendered as
  inverse).
- `TEvent`: mouse coordinates as `u8` (80×24 screen), `cmd` field added.
- Focus **per group** (no global pointer); the focusable list is walked when
  Tab is pressed (≤ 12 children).
- **Partial save-under**: drop-down menus save and restore their rectangle
  from the virtual buffer (free, 400 bytes at most). Dialogs still redraw the
  whole desktop (v1).
- Width detected at **run time** (`scr_cols`), buffers sized by
  `TUI_SCR_MAXCOLS` (compile for 40 to save about 1 KB).
- No `-Cl`: groups nest (desktop → window), so drawing and dispatch are
  re-entrant.

## Measurements (cc65 2.19, `-O -Os`)

Complete library (every module linked): 17,958 bytes of code, 200 of
constants, 3,709 of variables, and 3,064 in the language card (`LC`). An
application only links the modules it uses.

## Known limits (v1)

- No Shift-Tab (indistinguishable on the Apple II): Up/Left go backwards.
- No II/II+ (lowercase, MouseText and the alternate character set are
  assumed).
- No blinking cursor (alternate character set: no flash).
- Mouse: left button only, no double-click or drag. No page-2 double
  buffering. The library does not use auxiliary RAM (apart from the 80-column
  display); the editor does, for its text buffer.
- `TUI_MAX_CHILDREN` = 12 per group; only one `msgbox`/dialog at a time
  (`busy_show()` from `busy.h` shares this singleton too: do not call it while
  another dialog is open).


## File and folder selector (`filesel.h`)

One dialog to open a file, save under a new name or choose a folder. It
browses ProDOS directories (POSIX on a PC), never changes the program's
current directory and returns the full path. The library contains no visible
text: every label comes from the caller.

```c
static const TFileSelText txt = {
    "Name:", "OK", "Cancel", "New",
    "File exists.\nOverwrite it?",     /* FS_SAVE: overwrite confirmation */
    "Cannot create\nthe folder",       /* creation failure (FS_FOLDER) */
};

char path[FS_PATH_LEN] = "";           /* "" = current directory, or a suggested path */
if (filesel(FS_OPEN, "Open", &txt, path) == CM_OK)
    use(path);                         /* e.g. "/VOL/DIR/FILE" */
```

| Mode | Behavior |
| --- | --- |
| `FS_OPEN` | pick an existing file (Enter on a file in the list accepts it) |
| `FS_SAVE` | pick or type a name; asks for confirmation before overwriting |
| `FS_FOLDER` | folders only; "OK" returns the folder being shown, "New" creates the folder whose name is typed and enters it |

On the Apple II, names are upper-cased (ProDOS rule); the list shows at most
32 entries, with names of up to 15 characters. ProDOS directories are read in
39-byte pieces: no block buffer is reserved.

## Text editor (`apps/edit`)

An editor in the spirit of MS-DOS EDIT, built on the library: File / Edit /
Search / Help menus, selection, clipboard, search, replace, "go to line",
Open / Save As dialogs that browse ProDOS directories, mouse support (click,
drag to select). Files are ProDOS text files (type `TXT`, CR line endings; LF
and CRLF files and bit 7 are normalized when reading).

```
make edit-dsk                # build/edit.dsk (bootable ProDOS, with README.TXT)
make edit-dsk TUI_MOUSE=0    # no mouse: 3 KB more room for text
```

**Keyboard** (the Apple II has no Shift+arrow, no Home/End, no Page keys):

| Keys | Action |
| --- | --- |
| arrows | move |
| Solid Apple + movement | extend the selection |
| Open Apple + ← → / ↑ ↓ | previous/next word / page |
| `^A` `^E` / `^T` `^B` | start/end of line / start/end of text |
| Delete (`$7F`) / `^D` | delete left / right |
| `^X` `^C` `^V` `^L` `^Y` | cut, copy, paste, select all, delete line |
| `^F` `^G` `^R` `^W` | find, find next, replace, go to line |
| `^N` `^O` `^S` `^Q` | new, open, save, quit |
| Esc | menu bar; inside a menu, typing an item's initial runs it |

**Machine constraints** (measured, cc65 2.19):

- **Auxiliary RAM.** On a //c, or on a //e with an extended 80-column card
  (128 KB, which ProDOS reports in `MACHID`), the text buffer (a gap buffer)
  lives in auxiliary RAM (`$0800`–`$BFFF`): **47,104 bytes**, with no effect
  on main RAM. A 113-byte copy routine installed at `$0300` in both banks
  (`auxrt.s`) does the transfers, and a 256-byte window in main RAM serves as
  a read cache. Under ProDOS the `/RAM` disk occupies auxiliary memory: it is
  only taken over if it is **empty**, and it is then removed from the device
  list (it comes back at the next boot); if it holds files, the editor falls
  back to main RAM. `make edit-dsk EDIT_AUX=0` builds a version without this
  code.
- **Main RAM only** (64 KB //e, or a non-empty `/RAM`): the program occupies
  `$0C00` to `$A1D9` and the buffer takes all the free RAM above it, up to the
  stack: **6,438 bytes with mouse, 9,670 without** (8,363 with mouse and
  without the auxiliary code, `EDIT_AUX=0`).
- The free buffer size is always shown in the status bar (`Free`). A larger
  file is refused when opening ("File too large"), never truncated.
- The clipboard is 512 bytes.
- The 1 KB ProDOS file buffer is fixed at `$0800` (`apple2-iobuf-0800.o`),
  which removes the C heap (`malloc`); the code of the dialogs and of the file
  selector (about 3 KB, `LC` 99.7 % full) lives in the `LC` segment of the
  language card.

What comes next: see [ROADMAP.md](ROADMAP.md) (in French).
