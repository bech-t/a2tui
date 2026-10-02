/* editor.c -- TEditor : affichage, curseur, selection, edition. */

#include <string.h>
#include "editor.h"
#include "textbuf.h"
#include "screen.h"
#include "keyboard.h"

TEditor ed;

static u8  clip[CLIP_MAX];
static u16 clip_len;
static char lb[TUI_SCR_MAXCOLS + 1];

static void notify(void)
{
    if (ed.notify)
        ed.notify();
}

/* --- Redessin partiel ------------------------------------------------- */

static void mark(int a, int b)
{
    int t;
    if (a > b) { t = a; a = b; b = t; }
    if (a < 0) a = 0;
    if (b >= ed.v.h) b = ed.v.h - 1;
    if (a > b)
        return;
    if ((u8)a < ed.dlo) ed.dlo = (u8)a;
    if ((u8)b > ed.dhi) ed.dhi = (u8)b;
    view_invalidate(&ed.v);
}

static void mark_all(void)
{
    ed.dall = 1;
    view_invalidate(&ed.v);
}

/* Selection normalisee : 1 et [*s0,*s1) si une selection non vide existe. */
static u8 sel_range(u16 *s0, u16 *s1)
{
    if (!ed.sel || ed.anchor == ed.caret)
        return 0;
    if (ed.anchor < ed.caret) { *s0 = ed.anchor; *s1 = ed.caret; }
    else                      { *s0 = ed.caret;  *s1 = ed.anchor; }
    return 1;
}

u8 ed_has_sel(void)
{
    u16 a, b;
    return sel_range(&a, &b);
}

u16 ed_sel_len(void)
{
    u16 a, b;
    return sel_range(&a, &b) ? (u16)(b - a) : 0;
}

/* --- Dessin ------------------------------------------------------------ */

static void render_row(u8 r, u16 pos, u16 le, u8 tw, u8 ax, u8 ay,
                       u8 hassel, u16 s0, u16 s1)
{
    u16 base = (u16)(pos + ed.hscroll), n = 0;
    u8 c, lo = 0, hi = 0, inrow = 0, cc;
    u16 a, b;
    if (le > base)
        n = (u16)(le - base);
    if (n > tw)
        n = tw;
    {   /* copie par segments contigus, puis nettoyage des caracteres non affichables */
        u16 pp = base, k, left = n;
        char *d = lb;
        const u8 *src;
        while (left) {
            src = tb_ptr(pp, &k);
            if (k > left)
                k = left;
            memcpy(d, src, k);
            d += k;
            pp = (u16)(pp + k);
            left = (u16)(left - k);
        }
    }
    for (c = (u8)n; c < tw; ++c)
        lb[c] = ' ';

    if (hassel && s0 <= le && s1 > pos) {
        inrow = 1;
        a = s0 > base ? (u16)(s0 - base) : 0;
        b = s1 > base ? (u16)(s1 - base) : 0;
        if (a > tw) a = tw;
        if (b > tw) b = tw;
        lo = (u8)a;
        hi = (u8)b;
        if (lo > hi)
            lo = hi;
    }
    if (inrow && hi > lo) {
        scr_putsw(ax, ay + r, lb, lo, ATTR_NORMAL);
        scr_putsw(ax + lo, ay + r, lb + lo, (u8)(hi - lo), ATTR_INVERSE);
        scr_putsw(ax + hi, ay + r, lb + hi, (u8)(tw - hi), ATTR_NORMAL);
    } else {
        scr_putsw(ax, ay + r, lb, tw, ATTR_NORMAL);
    }
    /* curseur : cellule inversee (rendue normale a l'interieur d'une selection) */
    if (ed.line == (u16)(ed.top_line + r) && ed.col >= ed.hscroll &&
        ed.col < (u16)(ed.hscroll + tw)) {
        cc = (u8)(ed.col - ed.hscroll);
        scr_putc(ax + cc, ay + r, lb[cc],
                 (inrow && cc >= lo && cc < hi) ? ATTR_NORMAL : ATTR_INVERSE);
    }
}

static void ed_draw(TView *v)
{
    u8 ax, ay, r, tw = (u8)(v->w - 1), thumb;
    u16 pos = ed.top, le = 0, len = tb_len(), s0 = 0, s1 = 0;
    u8 exists = 1, hassel = sel_range(&s0, &s1);
    u8 full = ed.dall || (v->flags & VF_FULL);
    view_abs(v, &ax, &ay);
    for (r = 0; r < v->h; ++r) {
        if (exists)
            le = tb_line_end(pos);
        if (full || (r >= ed.dlo && r <= ed.dhi)) {
            if (exists)
                render_row(r, pos, le, tw, ax, ay, hassel, s0, s1);
            else
                scr_fill(ax, ay + r, tw, 1, ' ', ATTR_NORMAL);
        }
        if (exists) {
            if (le < len)
                pos = (u16)(le + 1);
            else
                exists = 0;
        }
    }
    /* barre de defilement : piste + curseur de position */
    thumb = ed.nlines > 1
          ? (u8)(((unsigned long)(ed.line - 1) * (v->h - 1)) / (ed.nlines - 1)) : 0;
    for (r = 0; r < v->h; ++r) {
        if (r == thumb)
            scr_putc(ax + tw, ay + r, ' ', ATTR_INVERSE);
        else
            scr_putc(ax + tw, ay + r, G_TRACK, ATTR_NORMAL);
    }
    ed.dall = 0;
    ed.dlo = 0xFF;
    ed.dhi = 0;
    v->flags &= ~(VF_DIRTY | VF_FULL);
}

/* --- Deplacement ------------------------------------------------------- */

/* Fait defiler la zone de texte de d lignes (d > 0 : le texte monte) puis
 * marque uniquement les lignes decouvertes. */
static void scroll_view(u8 d, u8 up)
{
    u8 ax, ay;
    view_abs(&ed.v, &ax, &ay);
    scr_scroll(ax, ay, ed.v.w, ed.v.h, up ? (signed char)d : (signed char)-(signed char)d);
    if (up)
        mark(ed.v.h - d, ed.v.h - 1);
    else
        mark(0, d - 1);
}

/* Garde le curseur visible : ajuste top/top_line et hscroll. */
static void ed_scroll(void)
{
    u8 back, i, tw = (u8)(ed.v.w - 1), d;
    u16 p, old_top_line = ed.top_line;
    if (ed.line < ed.top_line) {
        ed.top = tb_line_start(ed.caret);
        ed.top_line = ed.line;
        d = (u8)(old_top_line - ed.line < ed.v.h ? old_top_line - ed.line : ed.v.h);
        if (d < ed.v.h && !ed.dall)
            scroll_view(d, 0);
        else
            mark_all();
    } else if (ed.line >= (u16)(ed.top_line + ed.v.h)) {
        back = (u8)(ed.v.h - 1);
        p = tb_line_start(ed.caret);
        for (i = 0; i < back && p > 0; ++i)
            p = tb_line_start((u16)(p - 1));
        ed.top = p;
        ed.top_line = (u16)(ed.line - i);
        d = (u8)(ed.top_line - old_top_line < ed.v.h ? ed.top_line - old_top_line : ed.v.h);
        if (d < ed.v.h && !ed.dall)
            scroll_view(d, 1);
        else
            mark_all();
    }
    if (ed.col < ed.hscroll) {
        ed.hscroll = ed.col;
        mark_all();
    } else if (ed.col >= (u16)(ed.hscroll + tw)) {
        ed.hscroll = (u16)(ed.col - tw + 1);
        mark_all();
    }
}

/* Place le curseur en pos ; ext : etend la selection. keepwant : conserve la
 * colonne visee (deplacements verticaux). */
static void ed_move(u16 pos, u8 ext, u8 keepwant)
{
    u16 old_caret = ed.caret, old_line = ed.line, aline = ed.anchor_line, lo, hi;
    u8 had_sel = ed.sel;

    if (ext) {
        if (!ed.sel) {
            ed.sel = 1;
            ed.anchor = ed.caret;
            ed.anchor_line = ed.line;
        }
    } else {
        ed.sel = 0;
    }
    if (pos > old_caret)
        ed.line = (u16)(ed.line + tb_count_cr(old_caret, pos));
    else if (pos < old_caret)
        ed.line = (u16)(ed.line - tb_count_cr(pos, old_caret));
    ed.caret = pos;
    ed.col = (u16)(pos - tb_line_start(pos));
    if (!keepwant)
        ed.want_col = ed.col;
    ed_scroll();

    lo = old_line < ed.line ? old_line : ed.line;
    hi = old_line < ed.line ? ed.line : old_line;
    if (had_sel && !ext) {
        if (aline < lo) lo = aline;
        if (aline > hi) hi = aline;
    }
    mark((int)lo - (int)ed.top_line, (int)hi - (int)ed.top_line);
    view_invalidate(&ed.v);
    notify();
}

static u16 pos_up(u16 from)
{
    u16 ls = tb_line_start(from), pls, n;
    if (ls == 0)
        return from;
    pls = tb_line_start((u16)(ls - 1));
    n = (u16)(ls - 1 - pls);
    return (u16)(pls + (ed.want_col < n ? ed.want_col : n));
}

static u16 pos_down(u16 from)
{
    u16 le = tb_line_end(from), nle, n;
    if (le >= tb_len())
        return from;
    nle = tb_line_end((u16)(le + 1));
    n = (u16)(nle - le - 1);
    return (u16)(le + 1 + (ed.want_col < n ? ed.want_col : n));
}

static u8 is_word(u8 c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') || c == '_';
}

static u16 pos_word_left(void)
{
    u16 p = ed.caret;
    while (p > 0 && !is_word(tb_at((u16)(p - 1)))) --p;
    while (p > 0 && is_word(tb_at((u16)(p - 1)))) --p;
    return p;
}

static u16 pos_word_right(void)
{
    u16 p = ed.caret, len = tb_len();
    while (p < len && is_word(tb_at(p))) ++p;
    while (p < len && !is_word(tb_at(p))) ++p;
    return p;
}

/* --- Edition ----------------------------------------------------------- */

/* Efface [from,to) ; le curseur se retrouve en from. */
static void ed_erase(u16 from, u16 to)
{
    u16 crs = tb_count_cr(from, to);
    int row;
    ed_move(from, 0, 0);
    tb_delete(from, (u16)(to - from));
    ed.nlines = (u16)(ed.nlines - crs);
    ed.modified = 1;
    row = (int)ed.line - (int)ed.top_line;
    mark(row, crs ? ed.v.h - 1 : row);
    notify();
}

static void erase_sel(void)
{
    u16 s0, s1;
    if (sel_range(&s0, &s1))
        ed_erase(s0, s1);
    ed.sel = 0;
}

u8 ed_insert(const u8 *s, u16 n)
{
    u16 i, crs = 0, old_line;
    erase_sel();
    if (tb_free() < n)
        return 0;
    for (i = 0; i < n; ++i)
        if (s[i] == '\r')
            ++crs;
    old_line = ed.line;
    tb_insert(ed.caret, s, n);
    ed.caret = (u16)(ed.caret + n);
    ed.line = (u16)(ed.line + crs);
    ed.nlines = (u16)(ed.nlines + crs);
    ed.col = (u16)(ed.caret - tb_line_start(ed.caret));
    ed.want_col = ed.col;
    ed.modified = 1;
    ed.sel = 0;
    ed_scroll();
    mark((int)old_line - (int)ed.top_line,
         crs ? ed.v.h - 1 : (int)old_line - (int)ed.top_line);
    view_invalidate(&ed.v);
    notify();
    return 1;
}

void ed_backspace(void)
{
    if (ed_has_sel())
        erase_sel();
    else if (ed.caret > 0)
        ed_erase((u16)(ed.caret - 1), ed.caret);
}

void ed_delete(void)
{
    if (ed_has_sel())
        erase_sel();
    else if (ed.caret < tb_len())
        ed_erase(ed.caret, (u16)(ed.caret + 1));
}

void ed_delete_line(void)
{
    u16 ls = tb_line_start(ed.caret), le = tb_line_end(ed.caret), len = tb_len();
    if (le < len)
        ed_erase(ls, (u16)(le + 1));
    else if (ls > 0)
        ed_erase((u16)(ls - 1), le);
    else if (le > ls)
        ed_erase(ls, le);
}

/* --- Presse-papiers ---------------------------------------------------- */

u8 ed_copy(void)
{
    u16 s0, s1;
    if (!sel_range(&s0, &s1))
        return 1;
    if (s1 - s0 > CLIP_MAX)
        return 2;
    tb_copy(s0, (u16)(s1 - s0), clip);
    clip_len = (u16)(s1 - s0);
    return 0;
}

u8 ed_cut(void)
{
    u8 r = ed_copy();
    if (r == 0)
        erase_sel();
    return r;
}

u8 ed_paste(void)
{
    if (clip_len == 0)
        return 3;
    return ed_insert(clip, clip_len) ? 0 : 3;
}

void ed_clear_sel(void)
{
    if (ed_has_sel())
        erase_sel();
}

void ed_select_all(void)
{
    ed_move(0, 0, 0);
    ed_move(tb_len(), 1, 0);
}

/* --- Recherche --------------------------------------------------------- */

static void select_range(u16 a, u16 b)
{
    ed_move(a, 0, 0);
    ed_move(b, 1, 0);
}

u8 ed_find(const u8 *pat, u8 n, u8 match_case)
{
    u16 p = tb_find(ed.caret, pat, n, match_case);
    if (p == TB_NONE)
        p = tb_find(0, pat, n, match_case);
    if (p == TB_NONE)
        return 0;
    select_range(p, (u16)(p + n));
    return 1;
}

static u8 sel_matches(const u8 *pat, u8 n, u8 match_case)
{
    u16 s0, s1, i;
    u8 a, b;
    if (!sel_range(&s0, &s1) || s1 - s0 != n)
        return 0;
    for (i = 0; i < n; ++i) {
        a = tb_at((u16)(s0 + i));
        b = pat[i];
        if (!match_case) {
            if (a >= 'A' && a <= 'Z') a = (u8)(a + 32);
            if (b >= 'A' && b <= 'Z') b = (u8)(b + 32);
        }
        if (a != b)
            return 0;
    }
    return 1;
}

u8 ed_replace(const u8 *pat, u8 n, u8 match_case, const u8 *rep, u8 rn)
{
    if (!sel_matches(pat, n, match_case))
        return 0;
    erase_sel();
    if (rn)
        return ed_insert(rep, rn);
    return 1;
}

u16 ed_replace_all(const u8 *pat, u8 n, u8 match_case, const u8 *rep, u8 rn)
{
    u16 count = 0, p = 0;
    if (n == 0)
        return 0;
    for (;;) {
        p = tb_find(p, pat, n, match_case);
        if (p == TB_NONE)
            break;
        select_range(p, (u16)(p + n));
        if (!ed_replace(pat, n, match_case, rep, rn))
            break;                        /* tampon plein */
        ++count;
        p = ed.caret;
    }
    return count;
}

void ed_goto_line(u16 line)
{
    u16 pos = 0, len = tb_len(), cur = 1;
    if (line < 1) line = 1;
    if (line > ed.nlines) line = ed.nlines;
    while (cur < line && pos < len) {
        pos = (u16)(tb_line_end(pos) + 1);
        ++cur;
    }
    ed_move(pos, 0, 0);
}

/* --- Evenements -------------------------------------------------------- */

#if TUI_MOUSE
static u16 pos_at(u8 row, u16 col)
{
    u16 pos = ed.top, le, len = tb_len(), n;
    u8 r;
    for (r = 0; r < row; ++r) {
        le = tb_line_end(pos);
        if (le >= len)
            break;
        pos = (u16)(le + 1);
    }
    le = tb_line_end(pos);
    n = (u16)(le - pos);
    return (u16)(pos + (col < n ? col : n));
}
#endif

static u8 ed_handle(TView *v, TEvent *ev)
{
    u8 k, ext, big, ind, n;
    u16 p, ls;
    u8 buf[24];

    (void)v;
#if TUI_MOUSE
    if (ev->type == EV_MOUSE_DOWN || ev->type == EV_MOUSE_MOVE ||
        ev->type == EV_MOUSE_UP) {
        u8 ax, ay, row;
        int dy;
        view_abs(v, &ax, &ay);
        if (ev->type == EV_MOUSE_UP) {
            ed.dragging = 0;
            if (!ed_has_sel())
                ed.sel = 0;
            return EVENT_HANDLED;
        }
        if (ev->type == EV_MOUSE_MOVE && !(ed.dragging && (ev->buttons & 1)))
            return EVENT_NOT_HANDLED;
        dy = (int)ev->mouse_y - (int)ay;
        row = (u8)(dy < 0 ? 0 : (dy >= v->h ? v->h - 1 : dy));
        p = pos_at(row, (u16)(ed.hscroll + (ev->mouse_x > ax ? ev->mouse_x - ax : 0)));
        if (ev->type == EV_MOUSE_DOWN) {
            ed.dragging = 1;
            ed_move(p, (kbd_modifiers() & MOD_CLOSED_APPLE) != 0, 0);
        } else {
            ed_move(p, 1, 0);
        }
        return EVENT_HANDLED;
    }
#endif
    if (ev->type != EV_KEY)
        return EVENT_NOT_HANDLED;

    k = ev->key;
    ext = (ev->mods & MOD_CLOSED_APPLE) != 0;
    big = (ev->mods & MOD_OPEN_APPLE) != 0;

    if (k >= 0x20 && k < 0x7F) {
        buf[0] = k;
        ed_insert(buf, 1);
        return EVENT_HANDLED;
    }
    switch (k) {
    case KEY_ENTER:
        ls = tb_line_start(ed.caret);
        buf[0] = '\r';
        for (ind = 0; ind < 16 && (u16)(ls + ind) < ed.caret &&
                      tb_at((u16)(ls + ind)) == ' '; ++ind)
            buf[1 + ind] = ' ';
        ed_insert(buf, (u16)(1 + ind));
        break;
    case KEY_TAB:
        n = (u8)(4 - (ed.col & 3));
        memset(buf, ' ', n);
        ed_insert(buf, n);
        break;
    case KEY_DEL:
        ed_backspace();
        break;
    case KEY_CTRL('D'):
        ed_delete();
        break;
    case KEY_LEFT:
        if (big)   p = pos_word_left();
        else       p = ed.caret ? (u16)(ed.caret - 1) : 0;
        ed_move(p, ext, 0);
        break;
    case KEY_RIGHT:
        if (big)   p = pos_word_right();
        else       p = ed.caret < tb_len() ? (u16)(ed.caret + 1) : ed.caret;
        ed_move(p, ext, 0);
        break;
    case KEY_UP:
    case KEY_DOWN:
        p = ed.caret;
        for (n = big ? (u8)(ed.v.h - 1) : 1; n; --n) {
            u16 q = k == KEY_UP ? pos_up(p) : pos_down(p);
            if (q == p)
                break;
            p = q;
        }
        ed_move(p, ext, 1);
        break;
    case KEY_CTRL('A'):
        ed_move(tb_line_start(ed.caret), ext, 0);
        break;
    case KEY_CTRL('E'):
        ed_move(tb_line_end(ed.caret), ext, 0);
        break;
    case KEY_CTRL('T'):
        ed_move(0, ext, 0);
        break;
    case KEY_CTRL('B'):
        ed_move(tb_len(), ext, 0);
        break;
    default:
        return EVENT_NOT_HANDLED;
    }
    return EVENT_HANDLED;
}

static const TViewOps ed_ops = { ed_draw, ed_handle };

void ed_init(u8 x, u8 y, u8 w, u8 h)
{
    view_init(&ed.v, x, y, w, h, &ed_ops, VF_FOCUSABLE);
    ed.notify = 0;
    ed_reset();
}

void ed_reset(void)
{
    ed.caret = ed.anchor = 0;
    ed.sel = 0;
    ed.top = 0;
    ed.top_line = ed.line = ed.anchor_line = 1;
    ed.nlines = (u16)(tb_count_cr(0, tb_len()) + 1);
    ed.col = ed.want_col = ed.hscroll = 0;
    ed.modified = 0;
    ed.dragging = 0;
    mark_all();
    notify();
}
