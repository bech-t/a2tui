/* screen.c -- tampon virtuel, flush par lignes sales.
 *
 * Une cellule = un octet : directement le CODE ECRAN Apple II du jeu
 * alternatif ($A0-$FF normal = ASCII $20-$7F, $00-$3F et $60-$7F inverse,
 * $40-$5F MouseText). Le flush n'a ainsi rien a convertir. */

#include <string.h>
#include "screen.h"

#ifdef __CC65__
#include <apple2.h>
#define HWSW(a)     (*(volatile u8 *)(a) = 0)
#define HWRD(a)     (*(volatile u8 *)(a))
#define VRAM(a)     (*(volatile u8 *)(a))
#define HWPTR(a)    ((u8 *)(a))
#define HWPTR_AUX(a)  ((u8 *)(a))     /* meme adresse : la banque est choisie par PAGE2 */
#define HWPTR_MAIN(a) ((u8 *)(a))
#define SEL_AUX()   HWSW(0xC055)   /* PAGE2 on  -> $0400 = memoire auxiliaire */
#define SEL_MAIN()  HWSW(0xC054)   /* PAGE2 off -> $0400 = memoire principale */
#else
u8 tui_sim_main[1024], tui_sim_aux[1024];
u8 tui_sim_has_aux = 1, tui_sim_has_mousetext = 0;
unsigned long tui_sim_writes;
static u8 *sim_cur;
#define HWSW(a)     ((void)0)
#define HWRD(a)     0
#define VRAM(a)     (sim_cur[(a) - 0x400])
#define HWPTR(a)    (sim_cur + (a) - 0x400)
#define HWPTR_AUX(a)  (tui_sim_aux + (a) - 0x400)
#define HWPTR_MAIN(a) (tui_sim_main + (a) - 0x400)
#define SEL_AUX()   (sim_cur = tui_sim_aux)
#define SEL_MAIN()  (sim_cur = tui_sim_main)
#endif

#define STRIDE TUI_SCR_MAXCOLS

u8 scr_cols = 40;

static u8 vbuf[TUI_SCR_ROWS * STRIDE];     /* ce qu'on veut afficher */
static u8 dirty[TUI_SCR_ROWS];
static u16 roff[TUI_SCR_ROWS];             /* y * STRIDE, evite une multiplication */
static u16 rowbase[TUI_SCR_ROWS];          /* adresse physique de la ligne */
static u8 mode80;
static u8 cur_on, cur_x, cur_y;      /* pointeur de souris */
#if TUI_MOUSETEXT
static u8 mouse_text;
#endif

/* Entrelacement texte Apple II : $0400 + (y&7)*$80 + (y>>3)*$28. */
u16 scr_addr(u8 y)
{
    return (u16)0x0400 + (u16)(y & 7) * 0x80 + (u16)(y >> 3) * 0x28;
}

/* Repli ASCII des glyphes, indexe par (glyphe - 0x80). */
static const char glyph_ascii[G_COUNT] = {
    '+', '+', '+', '+', '-', '-', '|', '|', '-', '^', 'v', '<', '>', 'x', ':'
};

#if TUI_MOUSETEXT
/* Codes ecran MouseText (jeu alternatif). Bordure haute = le '_' normal
 * ($DF, trait en bas de cellule), bordure basse = $4C (trait en haut) : les
 * barres $5F/$5A des cotes les rejoignent sans coin. */
static const u8 glyph_mt[G_COUNT] = {
    0xDF, 0xDF, 0x4C, 0x4C, 0xDF, 0x4C, 0x5F, 0x5A,
    0x53, 0x4B, 0x4A, 0x48, 0x55, 0x44, 0x56
};
#endif

/* ASCII (ou glyphe >= 0x80) + attribut -> code ecran du jeu ALTERNATIF :
 * normal $A0-$FF, inverse $00-$1F ($40-$5F ASCII), $20-$3F et $60-$7F tels
 * quels. Les controles deviennent des espaces. */
static u8 encode(char c, u8 attr)
{
    u8 a = (u8)c;
    if (a & 0x80) {
        a &= 0x7F;
        if (a >= G_COUNT)
            a = ' ';
        else {
#if TUI_MOUSETEXT
            if (mouse_text && attr == ATTR_NORMAL)
                return glyph_mt[a];
#endif
            a = (u8)glyph_ascii[a];
        }
    }
    if (a < 0x20)
        a = 0x20;
    if (attr == ATTR_NORMAL)
        return (u8)(a | 0x80);
    if (a >= 0x40 && a < 0x60)
        a -= 0x40;
    return a;
}

static u8 detect_aux(void)
{
#ifdef __CC65__
    u8 mainv, auxv, ok;
    /* Test d'ecriture dans le "trou d'ecran" $0478 : faux sur II+/64 Ko.
     * On ne lit pas l'ID machine ($FBB3) : la ROM est banquee sous ProDOS. */
    HWSW(0xC001);                      /* 80STORE on : PAGE2 banque la page texte */
    SEL_MAIN();
    mainv = HWRD(0x0478);
    SEL_AUX();
    auxv = HWRD(0x0478);
    VRAM(0x0478) = 0x5A;
    SEL_MAIN();
    VRAM(0x0478) = 0xA5;
    SEL_AUX();
    ok = (HWRD(0x0478) == 0x5A);
    VRAM(0x0478) = auxv;
    SEL_MAIN();
    VRAM(0x0478) = mainv;
    HWSW(0xC000);                      /* 80STORE off */
    return ok;
#else
    return tui_sim_has_aux;
#endif
}

void scr_init(void)
{
    u8 y;
    for (y = 0; y < TUI_SCR_ROWS; ++y) {
        roff[y] = (u16)y * STRIDE;
        rowbase[y] = scr_addr(y);
    }
    /* Minimum //e / //c : jeu alternatif (inverse minuscule, MouseText) ;
     * ni II ni II+. */
    HWSW(0xC00F);
#if TUI_MOUSETEXT
#ifdef __CC65__
    mouse_text = (get_ostype() >= APPLE_IIEENH);   /* //e enhanced, carte //e, //c, IIgs */
#else
    mouse_text = tui_sim_has_mousetext;
#endif
#endif
#if TUI_SCR_MAXCOLS >= 80
    mode80 = detect_aux();
#else
    mode80 = 0;
#endif
    if (mode80) {
        HWSW(0xC00D);                  /* affichage 80 colonnes */
        HWSW(0xC001);                  /* 80STORE on */
        scr_cols = 80;
    } else {
        HWSW(0xC00C);
        HWSW(0xC000);
        scr_cols = 40;
    }
    HWSW(0xC051);                      /* texte */
    HWSW(0xC052);                      /* plein ecran */
    SEL_MAIN();                        /* page 1 */
    scr_fill(0, 0, scr_cols, TUI_SCR_ROWS, ' ', ATTR_NORMAL);
    scr_invalidate();
    scr_flush();
}

void scr_done(void)
{
    cur_on = 0;
    scr_cols = 40;
    HWSW(0xC00E);                      /* jeu primaire, comme au demarrage */
    HWSW(0xC00C);
    HWSW(0xC000);
    SEL_MAIN();
    memset(vbuf, 0xA0, sizeof vbuf);
    mode80 = 0;
    scr_invalidate();
    scr_flush();
}

/* Boucles internes : assembleur sur Apple II (screen_a.s), C sur PC. Les
 * parametres passent par ces variables (pas de pile logicielle). */
u8 *run_dst;
const char *run_src;
u8 run_n, run_code, run_aux;

#ifdef __CC65__
extern u8 put_run(void);
extern u8 fill_run(void);
extern void gather_run(void);
#else
static u8 put_run(void)
{
    u8 i, changed = 0, code, c, end = 0;
    for (i = 0; i < run_n; ++i) {
        c = end ? 0 : (u8)run_src[i];
        if (c == 0)
            end = 1;
        code = (c >= 0x20 && c < 0x7F) ? (u8)(c | 0x80) : 0xA0;
        if (run_dst[i] != code) {
            run_dst[i] = code;
            changed = 1;
        }
    }
    return changed;
}

static u8 fill_run(void)
{
    u8 i, changed = 0;
    for (i = 0; i < run_n; ++i)
        if (run_dst[i] != run_code) {
            run_dst[i] = run_code;
            changed = 1;
        }
    return changed;
}

static void gather_run(void)
{
    u8 i;
    for (i = 0; i < run_n; ++i)
        run_dst[i] = (u8)run_src[2 * i];
}
#endif

void scr_putc(u8 x, u8 y, char ch, u8 attr)
{
    u8 code;
    u16 i;
    if (x >= scr_cols || y >= TUI_SCR_ROWS)
        return;
    code = encode(ch, attr);
    i = roff[y] + x;
    if (vbuf[i] != code) {
        vbuf[i] = code;
        dirty[y] = 1;
    }
}

void scr_puts(u8 x, u8 y, const char *s, u8 attr)
{
    while (*s && x < scr_cols)
        scr_putc(x++, y, *s++, attr);
}

void scr_putsw(u8 x, u8 y, const char *s, u8 w, u8 attr)
{
    u8 *p;
    u8 code, changed = 0;
    if (y >= TUI_SCR_ROWS || x >= scr_cols)
        return;
    if ((u16)x + w > scr_cols)
        w = (u8)(scr_cols - x);
    p = vbuf + roff[y] + x;
    if (attr == ATTR_NORMAL) {           /* cas courant : boucle en assembleur */
        run_dst = p;
        run_src = s;
        run_n = w;
        if (put_run())
            dirty[y] = 1;
        return;
    }
    while (w--) {
        u8 c = (u8)*s;
        if (c)
            ++s;
        else
            c = ' ';
        code = encode((char)c, attr);
        if (*p != code) {
            *p = code;
            changed = 1;
        }
        ++p;
    }
    if (changed)
        dirty[y] = 1;
}

void scr_fill(u8 x, u8 y, u8 w, u8 h, char ch, u8 attr)
{
    if (x >= scr_cols)
        return;
    if ((u16)x + w > scr_cols)
        w = (u8)(scr_cols - x);
    run_code = encode(ch, attr);
    run_n = w;
    for (; h && y < TUI_SCR_ROWS; --h, ++y) {
        run_dst = vbuf + roff[y] + x;
        if (fill_run())
            dirty[y] = 1;
    }
}

void scr_box(u8 x, u8 y, u8 w, u8 h)
{
    u8 i, r = (u8)(x + w - 1), b = (u8)(y + h - 1);
    scr_fill(x, y, w, h, ' ', ATTR_NORMAL);
    for (i = (u8)(x + 1); i < r; ++i) {
        scr_putc(i, y, G_T, ATTR_NORMAL);
        scr_putc(i, b, G_B, ATTR_NORMAL);
    }
    for (i = (u8)(y + 1); i < b; ++i) {
        scr_putc(x, i, G_L, ATTR_NORMAL);
        scr_putc(r, i, G_R, ATTR_NORMAL);
    }
    scr_putc(x, y, G_TL, ATTR_NORMAL);
    scr_putc(r, y, G_TR, ATTR_NORMAL);
    scr_putc(x, b, G_BL, ATTR_NORMAL);
    scr_putc(r, b, G_BR, ATTR_NORMAL);
}

void scr_scroll(u8 x, u8 y, u8 w, u8 h, signed char n)
{
    u8 i, k = (u8)(n < 0 ? -n : n);
    if (k == 0 || k >= h || x >= scr_cols)
        return;
    if ((u16)x + w > scr_cols)
        w = (u8)(scr_cols - x);
    if (n > 0)
        for (i = 0; i + k < h; ++i)
            memmove(vbuf + roff[y + i] + x, vbuf + roff[y + i + k] + x, w);
    else
        for (i = (u8)(h - 1); i >= k; --i)
            memmove(vbuf + roff[y + i] + x, vbuf + roff[y + i - k] + x, w);
    for (i = 0; i < h; ++i)
        dirty[y + i] = 1;
}

void scr_save(u8 x, u8 y, u8 w, u8 h, u8 *buf)
{
    u8 j;
    for (j = 0; j < h; ++j) {
        memcpy(buf, vbuf + roff[y + j] + x, w);
        buf += w;
    }
}

void scr_restore(u8 x, u8 y, u8 w, u8 h, const u8 *buf)
{
    u8 j;
    for (j = 0; j < h; ++j) {
        memcpy(vbuf + roff[y + j] + x, buf, w);
        buf += w;
        dirty[y + j] = 1;
    }
}

void scr_invalidate(void)
{
    u8 y;
    for (y = 0; y < TUI_SCR_ROWS; ++y)
        dirty[y] = 1;
}

/* Une ligne en 40 colonnes : cellule x -> $base + x. */
static void flush_row40(u8 y)
{
    memcpy((void *)HWPTR(rowbase[y]), vbuf + roff[y], scr_cols);
#ifdef TUI_HOST
    tui_sim_writes += scr_cols;
#endif
}

/* Une ligne en 80 colonnes, colonnes de meme parite : la cellule x va en
 * $base + x/2 ; les paires en memoire auxiliaire, les impaires en principale. */
static void flush_row80(u8 y, u8 odd)
{
    run_dst = odd ? HWPTR_MAIN(rowbase[y]) : HWPTR_AUX(rowbase[y]);
    run_src = (const char *)(vbuf + roff[y] + odd);
    run_n = (u8)(scr_cols >> 1);
    run_aux = (u8)!odd;                  /* colonnes paires -> memoire auxiliaire */
    gather_run();
#ifdef TUI_HOST
    tui_sim_writes += scr_cols / 2;
#endif
}

/* Ecrit un code directement en memoire video (hors tampon). */
static void hw_put(u8 x, u8 y, u8 code)
{
    static u8 c;
    if (mode80 && !(x & 1)) {            /* colonne paire : memoire auxiliaire */
        c = code;
        run_dst = HWPTR_AUX(rowbase[y] + (x >> 1));
        run_src = (const char *)&c;
        run_n = 1;
        run_aux = 1;
        gather_run();
    } else {
        *HWPTR(rowbase[y] + (mode80 ? (x >> 1) : x)) = code;
    }
}

static u8 pointer_code(void)
{
#if TUI_MOUSETEXT
    if (mouse_text)
        return 0x42;                   /* fleche MouseText */
#endif
    return 0x20;                       /* espace inverse */
}

/* La ligne du pointeur est reecrite au prochain flush (efface l'ancien). */
void scr_mouse_move(u8 x, u8 y)
{
    if (x >= scr_cols) x = (u8)(scr_cols - 1);
    if (y >= TUI_SCR_ROWS) y = TUI_SCR_ROWS - 1;
    if (cur_on)
        dirty[cur_y] = 1;
    cur_x = x;
    cur_y = y;
    cur_on = 1;
}

void scr_mouse_show(u8 on)
{
    if (cur_on && !on)
        dirty[cur_y] = 1;
    cur_on = on;
}

void scr_flush(void)
{
    u8 y;
    if (mode80) {
        /* PAGE2 n'est actif que dans gather_run (voir screen_a.s). */
        for (y = 0; y < TUI_SCR_ROWS; ++y)
            if (dirty[y]) {
                flush_row80(y, 0);
                flush_row80(y, 1);
            }
    } else {
        for (y = 0; y < TUI_SCR_ROWS; ++y)
            if (dirty[y])
                flush_row40(y);
    }
    for (y = 0; y < TUI_SCR_ROWS; ++y)
        dirty[y] = 0;
    if (cur_on) {
        u8 code = pointer_code();
        hw_put(cur_x, cur_y, code);
#ifdef TUI_HOST
        ++tui_sim_writes;
#endif
    }
}
